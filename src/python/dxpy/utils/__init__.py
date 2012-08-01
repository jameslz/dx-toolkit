'''
Utilities shared by dxpy modules.
'''

import os, collections, concurrent.futures
from exec_utils import *

def response_iterator(request_iterator, worker_pool, max_active_tasks=4):
    '''
    :param request_iterator: This is expected to be an iterator producing inputs for consumption by the worker pool.
    :param worker_pool: Assumed to be a concurrent.futures.Executor instance.
    :max_active_tasks: The maximum number of tasks that may be either running or waiting for consumption of their result.

    Rate-limited asynchronous multithreaded task runner.
    Consumes tasks from *request_iterator*. Yields their results in order, while allowing up to *max_active_tasks* to run
    simultaneously. Unlike concurrent.futures.Executor.map, prevents new tasks from starting while there are
    *max_active_tasks* or more unconsumed results.
    '''
    future_deque = collections.deque()
    for i in range(max_active_tasks):
        try:
            _callable, args, kwargs = request_iterator.next()
            # print "Submitting (initial batch):", _callable, args, kwargs
            f = worker_pool.submit(_callable, *args, **kwargs)
            future_deque.append(f)
        except StopIteration:
            break

    while len(future_deque) > 0:
        f = future_deque.popleft()
        if not f.done():
            concurrent.futures.wait([f])
        if f.exception() is not None:
            raise f.exception()
        try:
            _callable, args, kwargs = request_iterator.next()
            # print "Submitting", _callable, args, kwargs
            next_future = worker_pool.submit(_callable, *args, **kwargs)
            future_deque.append(next_future)
        except StopIteration:
            pass
        yield f.result()

def string_buffer_length(buf):
    orig_pos = buf.tell()
    buf.seek(0, os.SEEK_END)
    buf_len = buf.tell()
    buf.seek(orig_pos)
    return buf_len

def normalize_timedelta(timedelta):
    '''
    Given a string like "1w" or "-5d", convert it to an integer in milliseconds.
    Note: not related to the datetime timedelta class.
    '''
    try:
        return int(timedelta)
    except ValueError:
        t, suffix = timedelta[:-1], timedelta[-1:]
        suffix_multipliers = {'s': 1000, 'm': 1000*60, 'h': 1000*60*60, 'd': 1000*60*60*24, 'w': 1000*60*60*24*7,
                              'm': 1000*60*60*24*30, 'y': 1000*60*60*24*365}
        if suffix not in suffix_multipliers:
            raise ValueError("Unrecognized timedelta "+str(timedelta))
        return int(t) * suffix_multipliers[suffix]
