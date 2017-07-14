import calendar
import datetime
import inspect
import json
import multiprocessing
import traceback

DAY_FORMAT = '%Y%m%d'
UNIQ_DAY_FORMAT = 'dt' + DAY_FORMAT


# assuming all date & datetime objects are UTC
def utc_to_timestamp(dt):
    return calendar.timegm(dt.timetuple())


timestamp_to_utc = datetime.datetime.utcfromtimestamp


def day_serialize(dtime):
    return dtime.strftime(UNIQ_DAY_FORMAT)


def day_deserialize(s):
    return datetime.datetime.strptime(s, UNIQ_DAY_FORMAT)


def str2date(s):
    return datetime.datetime.strptime(s, DAY_FORMAT).date()


def convert_keys_if_possible(dct):
    for str_key in dct.keys():
        try:
            int_key = int(str_key)
        except ValueError:
            break
        else:
            dct[int_key] = dct[str_key]
            del dct[str_key]
    return dct


class AutoSerialized(dict):
    def __init__(self, tpe, value):
        self.update({
            '_st_': tpe,  # type of serialized object
            '_sv_': value  # serialized value
        })

    @classmethod
    def extract_value(cls, dct):
        return dct['_sv_']

    @classmethod
    def extract_type(cls, dct):
        return dct['_st_']

    # returns None for missing type instead of an Exception
    @classmethod
    def get_type(cls, dct):
        return dct.get('_st_')


class SerializableDatetime(datetime.datetime):
    alias = 'dtime'

    def __dumpdict__(self):
        return AutoSerialized(
            SerializableDatetime.alias, utc_to_timestamp(self)
        )

    @classmethod
    def __loaddict__(cls, dct):
        return cls.utcfromtimestamp(
            AutoSerialized.extract_value(dct)
        )


class SerializableDate(datetime.date):
    alias = 'date'

    def __dumpdict__(self):
        return AutoSerialized(
            SerializableDate.alias, utc_to_timestamp(self)
        )

    @classmethod
    def __loaddict__(cls, dct):
        return cls.utcfromtimestamp(
            AutoSerialized.extract_value(dct)
        )


class SerializableSet(set):
    alias = 'set'

    def __dumpdict__(self):
        return AutoSerialized(
            SerializableSet.alias, tuple(self)
        )

    @classmethod
    def __loaddict__(cls, dct):
        return set(
            AutoSerialized.extract_value(dct)
        )


class SerializableFrozenset(set):
    alias = 'fset'

    def __dumpdict__(self):
        return AutoSerialized(
            SerializableFrozenset.alias, tuple(self)
        )

    @classmethod
    def __loaddict__(cls, dct):
        return frozenset(
            AutoSerialized.extract_value(dct)
        )


DESERIALIZERS = (
    SerializableDatetime, SerializableDate,
    SerializableSet, SerializableFrozenset
)

DESERIALIZERS_BY_ALIAS = {
    dserializer.alias: dserializer.__loaddict__
    for dserializer in DESERIALIZERS
}

DESERIALIZERS_BY_ALIAS.update({
    None: convert_keys_if_possible
})


def custom_loads(dct):
    return DESERIALIZERS_BY_ALIAS[AutoSerialized.get_type(dct)](dct)


class CustomEncoder(json.JSONEncoder):
    def default(self, obj):
        if hasattr(obj, '__dumpdict__'):
            return obj.__dumpdict__()
        # Let the base class default method raise the TypeError
        return super(CustomEncoder, self).default(obj)


class FileProtocol(object):
    @classmethod
    def dumps(cls, obj, debug=False):
        return json.dumps(
            obj, cls=CustomEncoder
        )

    @classmethod
    def loads(cls, json_text):
        try:
            return json.loads(json_text, object_hook=custom_loads)
        except ValueError as err:
            logger = multiprocessing.get_logger()
            logger.error('Corrupted json:\n%s' % json_text)
            traceback.print_exc(err)
            return []


class WorkerResults(FileProtocol):
    @classmethod
    def dumps_object(cls, obj, debug=False):
        '''
        NOTE: Please, check the keys of your dicts to be basic types only
        '''
        return cls.dumps(
            [
                pair
                for pair in inspect.getmembers(
                    obj, lambda m: not callable(m)
                )
                if not pair[0].startswith('_')
            ]
        )

    @classmethod
    def loads_object(cls, json_text):
        instance = WorkerResults()
        for key, value in cls.loads(json_text):
            setattr(instance, key, value)
        return instance


def to_unicode(obj):
    try:
        return unicode(obj)
    except UnicodeDecodeError:
        return unicode(obj, encoding='utf8')
