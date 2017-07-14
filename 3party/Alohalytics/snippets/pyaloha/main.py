import multiprocessing
import os
import sys

from pyaloha.protocol import str2date, WorkerResults
from pyaloha.worker import invoke_cmd_worker, load_plugin, setup_logs


def cmd_run(plugin_dir):
    """
    Main command line interface to pyaloha system
    """

    # TODO: argparse
    plugin_name = sys.argv[1]
    start_date, end_date = map(str2date, sys.argv[2:4])
    try:
        events_limit = int(sys.argv[4])
    except IndexError:
        events_limit = 0

    run(
        plugin_name,
        start_date, end_date,
        plugin_dir=plugin_dir, events_limit=events_limit
    )


def run(plugin_name, start_date, end_date, plugin_dir,
        data_dir='/mnt/disk1/alohalytics/by_date',
        results_dir='./stats',
        events_limit=0):
    """
    Pyaloha stats processing pipeline:
0. Load worker, aggregator, processor classes from a specified plugin (script)
1. Run workers (data preprocessors) on alohalytics files within specified range
2. Accumulate [and postprocess] worker results with an aggregator instance
3. Run stats processor and print results to stdout
    """

    aggregator = aggregate_raw_data(
        data_dir, results_dir, plugin_dir, plugin_name,
        start_date, end_date, events_limit
    )

    stats = load_plugin(
        plugin_name, plugin_dir=plugin_dir
    ).StatsProcessor(aggregator)

    logger = multiprocessing.get_logger()

    logger.info('Stats: processing')
    stats.process_stats()

    logger.info('Stats: outputting')
    stats.print_stats()

    logger.info('Stats: done')


def aggregate_raw_data(
        data_dir, results_dir, plugin_dir, plugin,
        start_date=None, end_date=None,
        events_limit=0,
        worker_num=3 * multiprocessing.cpu_count() / 4):
    """
    Workers-aggregator subpipeline:
0. Load worker, aggregator classes from a specified plugin
1. Run workers in parallel (basing on server stats files)
2. Accumulate results by an aggregator
3. Run aggregator post processing
    """
    setup_logs()
    logger = multiprocessing.get_logger()

    pool = multiprocessing.Pool(worker_num)

    try:
        items = (
            (plugin_dir, plugin, os.path.join(data_dir, fname), events_limit)
            for fname in os.listdir(data_dir)
            if check_fname(fname, start_date, end_date)
        )

        aggregator = load_plugin(
            plugin, plugin_dir=plugin_dir
        ).DataAggregator(results_dir)

        logger.info('Aggregator: aggregate')
        for results in pool.imap_unordered(invoke_cmd_worker, items):
            aggregator.aggregate(WorkerResults.loads_object(results))

        logger.info('Aggregator: post_aggregate')
        aggregator.post_aggregate(pool)

        logger.info('Aggregator: done')
    finally:
        pool.terminate()
        pool.join()

    return aggregator


def check_fname(filename, start_date, end_date):
    """
    Checking if a server stats file is the one we need to process
(depending on the server time dates).
    """
    if filename[0] == '.':
        return False
    return check_date(filename, start_date, end_date)


def check_date(filename, start_date, end_date):
    try:
        fdate = str2date(filename[-11:-3])
    except (ValueError, IndexError):
        raise Exception(
            'Unidentified alohalytics stats filename: %s' % filename
        )

    if start_date and fdate < start_date:
        return False
    if end_date and fdate > end_date:
        return False
    return True
