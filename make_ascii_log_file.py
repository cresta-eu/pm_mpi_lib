#!/usr/bin/env python 

# python make_ascii_log_file.py ./pmc/log.out 12

import sys
import os
import struct


def convertDataFile(fpath, field_cnt):

  ln_dict = {}

  with open(fpath, 'rb') as fb:
    while True:
      # assume field_cnt 8-byte reals per line
      bb = fb.read(field_cnt*8)
      if not bb:
        break

      unpack_format = 'd'*field_cnt
      vals = struct.unpack(unpack_format, bb)
      if not vals or field_cnt != len(vals):
        break

      rank = int(vals[0])
      tm = vals[1]

      ln = str(rank) + ' ' + str(tm)
      for i in range(2, field_cnt):
        ln += (' ' + str(int(vals[i])))

      if rank not in ln_dict:
        ln_dict[rank] = [ln]
      else:
        ln_dict[rank].append(ln)

  ln_dict_sorted = dict(sorted(ln_dict.items()))

  with open(fpath+'.ascii', 'w') as fa:
    fa.write("rank, time (s), step, substep, node (W), cpu (W), mem (W), node (J), cpu (J), mem (J), cpu0 (C), cpu1 (C)\n")
    for rank in ln_dict_sorted:
      for ln in ln_dict_sorted[rank]:
        fa.write(ln+'\n')

  return


fpath = sys.argv[1]
field_cnt = int(sys.argv[2])

print('Converting ' + fpath + ' to ASCII format...')
convertDataFile(fpath, field_cnt)
print('Done')
