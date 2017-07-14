# This is an example stats processing script that
# counts unique users and total events
# for the given period of server time
# USAGE: from the examples directory
# (processing only one server day stats file):
# python2.7 run.py count_users_and_events 20150401 20150401

import collections

from pyaloha.base import DataAggregator as BaseDataAggregator
from pyaloha.base import DataStreamWorker as BaseDataStreamWorker
from pyaloha.base import StatsProcessor as BaseStatsProcessor
from pyaloha.protocol import day_serialize


USERS_AND_EVENTS_HEADER = '''\
Users & Events by days
Date\tUsers\tEvents
'''


def info_item():
    return {
        'users': set(),
        'events': 0
    }


class DataStreamWorker(BaseDataStreamWorker):
    """
    This is a class representing a worker that collects unique users and
total events within given sequence of events (one by one).
    This worker is not guaranteed to have all available data
    @method process_unspecified is a generic method called on every event
despite of its actual type in its own process
    """
    def __init__(self):
        super(DataStreamWorker, self).__init__()

        self.dates = collections.defaultdict(info_item)

    def process_unspecified(self, event):
        dte = day_serialize(event.event_time.dtime.date())
        if event.event_time.is_accurate:
            self.dates[dte]['users'].add(event.user_info.uid)
            self.dates[dte]['events'] += 1


class DataAggregator(BaseDataAggregator):
    """
    This is a 'singleton' class that accumulates results from the workers.
    @method aggregate is called every time a worker is done with its events
    """
    def __init__(self, *args, **kwargs):
        super(DataAggregator, self).__init__(
            *args, **kwargs
        )

        self.dates = collections.defaultdict(info_item)

    def aggregate(self, results):
        for dte, stats in results.dates.iteritems():
            self.dates[dte]['users'].update(stats['users'])
            self.dates[dte]['events'] += stats['events']


class StatsProcessor(BaseStatsProcessor):
    """
    This is a stats results processor and representator. It is instantiated
with a pointer to aggregator that has already been done with all workers and
his postprocessing.
    Class can be used as a business logic processor
(it is guaranteed to have full data) or just as a pretty printer.
    @method gen_stats is called at the end of the stats processing pipeline and
provides one or several stats sections - each with a human-readable text header
and a sequence of sequence objects interpreted as a table of values
    """
    def gen_stats(self):
        yield USERS_AND_EVENTS_HEADER, (
            (dte, len(stats['users']), stats['events'])
            for dte, stats in sorted(
                self.aggregator.dates.iteritems()
            )
        )
