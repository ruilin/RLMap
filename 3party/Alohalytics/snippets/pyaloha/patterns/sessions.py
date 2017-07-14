import datetime

from pyaloha.protocol import day_serialize
from pyaloha.patterns.daily_over_fs import (
    DailyWorkerMixin,
    DataAggregator as DailyAggregator, list_extension_post_aggregate,
    StatsProcessor as DailyStatsProcessor
)


class Session(dict):
    def __init__(self, start, mode, is_broken=False):
        self.update({
            'mode': mode,
            'is_broken': is_broken,
            'start': start,
            'end': None,
            'content': []
        })

    def is_in_progress(self):
        return self['end'] is None


class SessionWorkerMixin(DailyWorkerMixin):
    def __init__(self, *args, **kwargs):
        super(SessionWorkerMixin, self).__init__(*args, **kwargs)

        self._users2modes = {}

    def get_session_history(self, dtime, uid):
        return self.data_per_days[day_serialize(dtime)].setdefault(uid, [])

    def get_last_open_session(self, dtime, uid):
        dt = day_serialize(dtime)
        try:
            return self.data_per_days[dt][uid][-1]
        except (KeyError, IndexError) as no_sessions:
            # session may break at midnight as we're looking on daily basis
            prev_dt = day_serialize(dtime - datetime.timedelta(days=1))
            try:
                return self.data_per_days[prev_dt][uid][-1]
            except (KeyError, IndexError) as no_prev_day_sessions:
                # we assume we have no sessions
                # with more than a 24-hour duration
                return

    def create_broken_session(self, dtime, uid):
        dt = day_serialize(dtime)

        session = Session(
            start=dtime, mode=self._users2modes.get(uid, None),
            is_broken=True
        )

        self.data_per_days[dt].setdefault(uid, []).append(session)

        return session

    # returns True if internal must process this event as well
    def process_boundary(self, event):
        raise NotImplementedError()

    def process_internal(self, event):
        raise NotImplementedError()

    def process_unspecified(self, event):
        if event.event_time.is_accurate and self.process_boundary(event):
            self.process_internal(event)


class SessionAggregator(DailyAggregator):
    post_aggregate_worker = staticmethod(list_extension_post_aggregate)


class SessionStatsProcessor(DailyStatsProcessor):
    pass
