#!/usr/bin/python

import sys

# pass the reference file path and the output file path as parameters

reference_file = sys.argv[1]
output_file = sys.argv[2]

# Column name and tolerance (if the colum is a float)
columns = [
    ['contig'],
    ['position'],
    ['reference_kmer'],
    ['read_index'],
    ['strand'],
    ['event_index'],
    ['event_level_mean', 0.05],
    ['event_stdv', 0.05],
    ['event_length'],
    ['model_kmer'],
    ['model_mean', 0.05],
    ['model_stdv', 0.05],
    ['standardized_level', 0.05]
]

line = 1

with open(reference_file) as reference, open(output_file) as output:
    # Skip header
    next(reference)
    next(output)

    line += 1

    for reference_line, output_line in zip(reference, output):
        # Line to array of values
        reference_line = reference_line.split()
        output_line = output_line.split()

        for (column, ref_value, out_value) in \
                zip(columns, reference_line, output_line):

            # Is a float value
            if len(column) > 1:
                if abs(float(ref_value) - float(out_value)) > column[1]:
                    print('Line ', line, ' => ', column[0], ', abs(',
                          ref_value, ' - ', out_value, ') >', column[1])
                    sys.exit(1)
            else:
                if ref_value != out_value:
                    print('Line ', line, ' => ', column[0], ', ',
                          ref_value, ' != ', out_value)
                    sys.exit(1)

        line += 1
