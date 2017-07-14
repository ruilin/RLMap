"""
    This is a useful stats processing pattern when data is processed
in a daily fashion, accumulated into day files (minimizing memory usage),
after that files are postprocessed and saved again.
    Then accumulated and postprocessed data is read from fs
in a date ascending order and given to a daily stats subscribers (
each solving one particular problem like DAU calculation).
    Finally subscribers are called to generate stats reports.

    By default (if you don't overload any methods):
    1) Aggregator uses data_per_days dict field of a worker as a
preprocessed stats data to be dumped into the files.
Its keys are serialized dates and values are another dicts.
    {
        date_str: {
            some_key: serializable_object
        }
    }
For example:
    {
        date_str: {
            uid: event
        }
    }
    2) Aggregator postprocesses files and leaves only one (arbitrary)
object for a key within one date.

"""

import collections
import itertools
import multiprocessing
import os

from pyaloha.base import DataAggregator as BaseDataAggregator
from pyaloha.base import StatsProcessor as BaseStatsProcessor
from pyaloha.base import ShareableData

from pyaloha.protocol import FileProtocol, day_deserialize, str2date


def brute_post_aggregate(fname):
    results = DataAggregator.load_day(fname)
    reduced = dict(results)
    DataAggregator.save_day(
        fname=fname,
        iterable=reduced.iteritems()
    )


def list_extension_post_aggregate(fname):
    results = DataAggregator.load_day(fname)

    reduced = collections.defaultdict(list)
    for key, _list in results:
        reduced[key].extend(_list)

    DataAggregator.save_day(
        fname=fname,
        iterable=reduced.iteritems()
    )


def setup_daily_data(self, *args, **kwargs):
    ShareableData.setup_shareable_data(self, *args, **kwargs)

    self.data_per_days = collections.defaultdict(dict)


class DailyWorkerMixin(object):
    setup_shareable_data = setup_daily_data


class DataAggregator(BaseDataAggregator):
    post_aggregate_worker = staticmethod(brute_post_aggregate)
    setup_shareable_data = setup_daily_data

    def aggregate(self, processor_results):
        self.logger.info(
            "Aggregator: got %d dates from worker" % len(
                processor_results.data_per_days
            )
        )

        for dte, data_dict in processor_results.data_per_days.iteritems():
            self.save_day(
                fname=self.get_result_file_path(day_deserialize(dte)),
                iterable=data_dict.iteritems(),
                mode='a+'
            )

        self.lost_data.update(processor_results.lost_data)

    def post_aggregate(self, pool=None):
        self.logger.warn(
            "daily_over_fs: lost keys: %s" % (
                len(self.lost_data)
            )
        )

        del self.lost_data

        engine = itertools.imap
        if pool:
            engine = pool.imap_unordered

        for _ in engine(
                self.post_aggregate_worker,
                self.iterate_saved_days()):
            pass

    def get_result_file_path(self, dte):
        if not self.results_dir:
            raise Exception('Results dir is not set to use this method')

        month_dir = os.path.join(
            self.results_dir, str(dte.year), str(dte.month)
        )
        if month_dir not in self.created_dirs:
            try:
                os.makedirs(month_dir)
            except OSError as error:
                self.logger.warn(
                    "daily_over_fs: result subdir '%s' creation failed: %s" % (
                        month_dir, error
                    )
                )
            else:
                self.created_dirs.add(month_dir)

        return os.path.join(month_dir, str(dte.day))

    def extract_date_from_path(self, path):
        dte = ''.join(map(
            lambda s: s.zfill(2),
            path.split('/')[-3:])
        )
        return str2date(dte)

    @staticmethod
    def save_day(iterable, fname, mode='w'):
        full_data_len = 0
        with open(fname, mode) as fout:
            for obj in iterable:
                try:
                    data = FileProtocol.dumps(obj) + '\n'
                except TypeError:
                    raise Exception(
                        "Object can't be serialized: %s" % repr(obj)
                    )
                fout.write(data)
                full_data_len += len(data)
        multiprocessing.get_logger().info(
            "daily_over_fs: wrote %d characters to %s" % (
                full_data_len, fname
            )
        )

    @staticmethod
    def load_day(fname):
        with open(fname) as fin:
            for line in fin:
                yield FileProtocol.loads(line)

    def iterate_saved_days(self):
        for root, dirs, fnames in os.walk(self.results_dir):
            for fn in fnames:
                yield os.path.join(root, fn)


class StatsSubscriber(object):
    def collect(self, item):
        raise NotImplementedError()

    def gen_stats(self):
        raise NotImplementedError()


class StatsProcessor(BaseStatsProcessor):
    subscribers = tuple()

    def prepare_collectable_item(self, fname):
        data_list = self.aggregator.load_day(fname)
        return self.aggregator.extract_date_from_path(fname), data_list

    def processable_iterator(self):
        return sorted(
            itertools.imap(
                self.prepare_collectable_item,
                self.aggregator.iterate_saved_days()
            )
        )

    def process_stats(self):
        self.procs = tuple(s() for s in self.subscribers)

        for dte, data_generator in self.processable_iterator():
            data_tuple = tuple(data_generator)
            for p in self.procs:
                p.collect((dte, data_tuple))

    def gen_stats(self):
        for p in self.procs:
            yield p.gen_stats()
