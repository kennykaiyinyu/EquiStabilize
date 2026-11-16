#!/usr/bin/env python3
"""
MLTWAP end-time buffer report (Aki logs).

Question it answers: if the TWAP stops trading N seconds before the user's
end time, is that enough to get the order done?  Per order it pulls the
end time, size, initial market width, every slice's ExecTime, the last fill
and the terminal state, then groups by liquidity (market width) and size.

Usage:
    python3 mltwap_endtime_report.py Aki_2026*.log  [--buffer 30] [--out report]
    python3 mltwap_endtime_report.py one.log --debug A000B862200000000

Outputs <out>_orders.csv (one row per order) and <out>_summary.csv, plus a
summary printed to the console.  Standard library only, Python 3.7+.
"""
import argparse, csv, glob, re, statistics, sys
from collections import defaultdict
from datetime import datetime, timedelta

# ---------- line patterns (taken from the Aki MLTWAP log) ----------
LINE = re.compile(
    r'^\w:\s+(\d{8})-(\d\d:\d\d:\d\d)\.(\d+)[.\d]*\([^)]*\)\s*\[(\d+)\]\[([\w.]+):(\d+)\]\s?(.*)$')
ALGO_ID = re.compile(r'\bAlgo(?:ID)?:?\s+(A[0-9A-F]{10,})')
LIMIT = re.compile(r'Algo (A[0-9A-F]+) Limt Price is ([-\d.]+)')
CALC_WIDTH = re.compile(r'CalcTWAP AlgoID (A[0-9A-F]+), Bid: (-?\d+), Ask: (-?\d+), MktWidth: (\d+)')
CALC_END = re.compile(r'CalcTWAP TWAP EndTime (\d+:\d+:\d+), TimeNow (\d+:\d+:\d+), Diff Seconds (-?\d+)')
SLICE_DONE = re.compile(r'Finished TWAPSlice\. AlgoID:?\s*(A[0-9A-F]+),\s*SliceQty\s*(\d+).*?Algo CumQty:?\s*(\d+)')
SLICE_STATS = re.compile(r'TWAPSlice Stats, ExecTime (\d+).*?MktWidth at Completion (-?\d+)')
FILL = re.compile(r'Updating TWAP CumQty\. AlgoID:?\s*(A[0-9A-F]+)')
STATE = re.compile(r'AlgoState -> (\w+) \[state-change\]')
TWAP_END_TAG = re.compile(r'TwapEndTime\|(\d\d:\d\d:\d\d)')
HARD_STOP = re.compile(r'approaching Hard Stop \(Diff (\d+) sec\)')
FULLY_FILLED = re.compile(r'Algo Fully Filled')
SLICE_SUMMARY = re.compile(r'TWAP Slice -\s*(\d+), Size:\s*(\d+), CumQty :\s*(\d+), Ask Percentage:\s*(-?\d+), Duration :\s*(\d+)')
SENDING_WAVES = re.compile(r'MLTWAP, Sending Waves')


def fix_tags(msg):
    sep = '\x01' if '\x01' in msg else '|'
    tags = defaultdict(list)
    for part in msg.split(sep):
        k, eq, v = part.partition('=')
        if eq and k.isdigit():
            tags[k].append(v)
    return tags


def new_order():
    return dict(algo_id=None, date=None, underlier='', qty=None, limit=None,
                end_local=None, start_ts=None, utc_off=None, width0=None,
                diff0=None, slices=[], cum=0, last_fill_ts=None,
                states=[], first_ts=None, hs_ts=None, hs_diff=None, hs_hits=0,
                hs_waves=0, done_ts=None, slice_summary={})


def parse(files, debug_id=None):
    orders = defaultdict(new_order)
    cur_algo = {}            # thread id -> last algo id seen on that thread
    pending_inbound = {}     # thread id -> (ts, tags) until the algo id shows up
    for path in files:
        with open(path, 'r', errors='replace') as fh:
            for raw in fh:
                m = LINE.match(raw.rstrip('\n'))
                if not m:
                    continue
                d, t, ms, thread, src, _ln, body = m.groups()
                ts = datetime.strptime(d + t + ms[:6], '%Y%m%d%H:%M:%S%f')

                if src.startswith('InboundOrderManager') and '15001=MLTWAP' in body:
                    pending_inbound[thread] = (ts, body)
                    continue
                if not src.startswith('MLTWAP'):
                    continue

                ids = ALGO_ID.findall(body)
                if ids:
                    cur_algo[thread] = ids[0]
                aid = cur_algo.get(thread)
                if not aid:
                    continue
                o = orders[aid]
                o['algo_id'] = aid
                o['first_ts'] = o['first_ts'] or ts
                if debug_id and aid == debug_id:
                    print('DEBUG', raw.rstrip())

                if thread in pending_inbound and (mm := LIMIT.search(body)):
                    its, msg = pending_inbound.pop(thread)
                    tags = fix_tags(msg)
                    o['date'] = d
                    o['start_ts'] = its
                    o['qty'] = int(float(tags['38'][0])) if tags.get('38') else None
                    legs = tags.get('600') or []
                    o['underlier'] = legs[0] if legs else ''
                    if (e := TWAP_END_TAG.search(msg)):
                        o['end_local'] = e.group(1)
                    o['limit'] = float(mm.group(2))
                elif (mm := CALC_WIDTH.search(body)) and o['width0'] is None:
                    o['width0'] = int(mm.group(4))
                elif (mm := CALC_END.search(body)):
                    end, now, diff = mm.groups()
                    if o['utc_off'] is None:
                        nowt = datetime.strptime(d + now, '%Y%m%d%H:%M:%S')
                        o['utc_off'] = round((ts - nowt).total_seconds() / 3600)
                        o['diff0'] = int(diff)
                    o['end_local'] = o['end_local'] or end
                elif (mm := SLICE_DONE.search(body)):
                    o['cum'] = max(o['cum'], int(mm.group(3)))
                    o['slices'].append({'ts': ts, 'qty': int(mm.group(2)), 'exec': None})
                elif (mm := SLICE_STATS.search(body)):
                    if o['slices'] and o['slices'][-1]['exec'] is None:
                        o['slices'][-1]['exec'] = int(mm.group(1))
                elif FILL.search(body):
                    o['last_fill_ts'] = ts
                if (mm := HARD_STOP.search(body)):
                    o['hs_hits'] += 1
                    if o['hs_ts'] is None:
                        o['hs_ts'], o['hs_diff'] = ts, int(mm.group(1))
                elif o['hs_ts'] and SENDING_WAVES.search(body):
                    o['hs_waves'] += 1
                elif (mm := SLICE_SUMMARY.search(body)):
                    n, size, cq, askp, dur = map(int, mm.groups())
                    o['slice_summary'][n] = (size, cq, askp, dur)
                if FULLY_FILLED.search(body):
                    o['done_ts'] = o['done_ts'] or ts
                if (mm := STATE.search(body)):
                    o['states'].append((ts, mm.group(1)))
    return orders


def to_row(o, buffer_s):
    off = timedelta(hours=o['utc_off'] if o['utc_off'] is not None else 5)
    end_ts = None
    if o['end_local'] and o['date']:
        end_ts = datetime.strptime(o['date'] + o['end_local'], '%Y%m%d%H:%M:%S') + off
    execs = [s['exec'] for s in o['slices'] if s['exec'] is not None]
    final_state, final_ts = (o['states'][-1][1], o['states'][-1][0]) if o['states'] else ('', None)
    secs = lambda a: round((end_ts - a).total_seconds(), 1) if (end_ts and a) else ''
    planned_buffer = ''
    if end_ts and o['start_ts'] and o['diff0'] is not None:
        planned_buffer = round((end_ts - o['start_ts']).total_seconds() - o['diff0'])
    qty = o['qty'] or 0
    durs = [v[3] for _, v in sorted(o['slice_summary'].items())]
    hs_to_done = ''
    if o['hs_ts'] and o['done_ts']:
        hs_to_done = round((o['done_ts'] - o['hs_ts']).total_seconds(), 3)
    return {
        'date': o['date'], 'algo_id': o['algo_id'], 'underlier': o['underlier'],
        'qty': o['qty'], 'limit': o['limit'], 'mkt_width0': o['width0'],
        'end_time_local': o['end_local'],
        'duration_s': round((end_ts - o['start_ts']).total_seconds()) if end_ts and o['start_ts'] else '',
        'planned_buffer_s': planned_buffer,
        'filled': o['cum'], 'fill_pct': round(100 * o['cum'] / qty, 1) if qty else '',
        'n_slices': len(o['slices']),
        'max_slice_exec_s': max(execs) if execs else '',
        'last_slice_exec_s': execs[-1] if execs else '',
        'last_fill_s_before_end': secs(o['last_fill_ts']),
        'final_state': final_state, 'final_state_s_before_end': secs(final_ts),
        f'slices_over_{buffer_s}s': sum(1 for e in execs if e > buffer_s),
        'completed': bool(o['done_ts']),
        'done_s_before_end': secs(o['done_ts']),
        'hard_stop_hit': bool(o['hs_ts']),
        'hard_stop_diff_s': o['hs_diff'] if o['hs_diff'] is not None else '',
        'hard_stop_to_done_s': hs_to_done,
        'hard_stop_waves_sent': o['hs_waves'] if o['hs_ts'] else '',
        'slice_durations_s': ' '.join(map(str, durs)),
        'max_slice_duration_s': max(durs) if durs else '',
    }


def bucket_width(w):
    if w in ('', None): return 'unknown'
    return 'tight(<=10)' if w <= 10 else 'mid(11-50)' if w <= 50 else 'wide(>50)'


def bucket_qty(q):
    if not q: return 'unknown'
    return 'small(<=10)' if q <= 10 else 'mid(11-100)' if q <= 100 else 'large(>100)'


def pct(xs, p):
    xs = sorted(xs)
    return xs[min(len(xs) - 1, int(round(p / 100 * (len(xs) - 1))))] if xs else ''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('logs', nargs='+')
    ap.add_argument('--buffer', type=int, default=30)
    ap.add_argument('--out', default='mltwap_buffer')
    ap.add_argument('--include-early-cancels', action='store_true',
                    help='keep orders the user cancelled well before end time')
    ap.add_argument('--debug', metavar='ALGO_ID')
    a = ap.parse_args()

    files = sorted({f for p in a.logs for f in glob.glob(p)})
    if not files:
        sys.exit('no log files matched')
    rows = [to_row(o, a.buffer) for o in parse(files, a.debug).values() if o['start_ts']]
    if not rows:
        sys.exit('no MLTWAP orders found - run with --debug <ALGO_ID> to check the patterns')

    with open(a.out + '_orders.csv', 'w', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]))
        w.writeheader(); w.writerows(rows)

    def early_cancel(r):
        s = r['final_state_s_before_end']
        return r['final_state'] == 'UsrCxl' and s != '' and s > 63 + 5

    use = rows if a.include_early_cancels else [r for r in rows if not early_cancel(r)]
    groups = defaultdict(list)
    for r in use:
        groups[(bucket_width(r['mkt_width0']), bucket_qty(r['qty']))].append(r)

    summ = []
    B = a.buffer
    for (wb, qb), rs in sorted(groups.items()):
        hs = [r for r in rs if r['hard_stop_hit']]
        hs_done = [r['hard_stop_to_done_s'] for r in hs if r['hard_stop_to_done_s'] != '']
        md = [r['max_slice_duration_s'] for r in rs if r['max_slice_duration_s'] != '']
        fp = [r['fill_pct'] for r in rs if r['fill_pct'] != '']
        summ.append({
            'width_bucket': wb, 'qty_bucket': qb, 'orders': len(rs),
            'pct_completed': round(100 * sum(r['completed'] for r in rs) / len(rs), 1),
            'orders_hit_hard_stop': len(hs),
            'hard_stop_not_completed': sum(1 for r in hs if not r['completed']),
            'hs_to_done_median_s': statistics.median(hs_done) if hs_done else '',
            'hs_to_done_p90_s': pct(hs_done, 90),
            'hs_to_done_max_s': max(hs_done) if hs_done else '',
            f'pct_hs_done_within_{B}s': round(100 * sum(1 for x in hs_done if x <= B) / len(hs), 1) if hs else '',
            'max_slice_duration_median_s': statistics.median(md) if md else '',
            'max_slice_duration_p90_s': pct(md, 90),
            'median_fill_pct': statistics.median(fp) if fp else '',
        })
    with open(a.out + '_summary.csv', 'w', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=list(summ[0]))
        w.writeheader(); w.writerows(summ)

    print(f'{len(files)} file(s), {len(rows)} orders, {len(rows) - len(use)} early user-cancels excluded')
    for s in summ:
        print(' | '.join(f'{k}={v}' for k, v in s.items()))
    print(f'wrote {a.out}_orders.csv and {a.out}_summary.csv')


if __name__ == '__main__':
    main()
