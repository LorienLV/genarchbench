import os
import sys
from time import time
import numpy as np
import deepdish as dd
import tensorflow as tf
# import shared.param as param
from clair3.model import Clair3_F
from argparse import ArgumentParser


def prediction(args, m):

    print("Begin predicting...")
    prediction_output = []
    input_mini_match = dd.io.load(args.input_fn)
    reference_mini_match = dd.io.load(args.reference_fn)
    time_counter = {"Load_mini_batch": [],
                    "Model_prediction": [],
                    "Write_batch_to_output": []}

    begin_time = time()
    for i in range(len(input_mini_match)):
        mini_batch = input_mini_match[i]
        X, _ = mini_batch
        tmp_time = time()
        prediction = m.predict_on_batch(X)
        cost_time = time() - tmp_time
        # #print(cost_time)
        time_counter["Model_prediction"].append(round(cost_time, 4))
        prediction_output.append(prediction)

    end_time = time() - begin_time

    comp = []
    for i in range(len(input_mini_match)):
        print(prediction_output[i], reference_mini_match[i][0])
        # print(len(prediction_output[i]), len(reference_mini_match[i][1]))
        # print(prediction_output[i])
        comp.append(np.all(np.round(prediction_output[i][0], 3) == np.round(reference_mini_match[i][0], 3)))

    print(comp)
    #if False not in comp:
    #    print("My_prediction function is correct, which takes %.4f s" % end_time)
    #else:
    #    print("My_prediction function is wrong, which takes %.4f s" % end_time)
    #dd.io.save("time_counter_my_prediction.h5", time_counter)
    print("Time taken: %.4f s" % end_time)

def Run(args):

    os.environ["OMP_NUM_THREADS"] = "1"
    os.environ["OPENBLAS_NUM_THREADS"] = "1"
    os.environ["MKL_NUM_THREADS"] = "1"
    os.environ["MKL_NUM_THREADS"] = "1"
    os.environ["NUMEXPR_NUM_THREADS"] = "1"
    os.environ["CUDA_VISIBLE_DEVICES"] = ""

    tf.config.threading.set_intra_op_parallelism_threads(1)
    tf.config.threading.set_inter_op_parallelism_threads(1)

    m = Clair3_F(add_indel_length=False, predict=True)
    m.load_weights(args.chkpnt_fn)

    prediction(args, m)


def main():
    parser = ArgumentParser(description="Call variants using a trained model and tensors of candididate variants")

    parser.add_argument('--input_fn', type=str, default="prediction_input.h5",
                        help="Input file (.h5)")

    parser.add_argument('--reference_fn', type=str, default="prediction_output.h5",
                        help="Reference output (.h5)")

    parser.add_argument('--chkpnt_fn', type=str, default=None,
                        help="Input a checkpoint for testing")

    parser.add_argument('--qual', type=int, default=None,
                        help="If set, variant with equal or higher quality will be marked PASS, or LowQual otherwise, optional")

    parser.add_argument('--threads', type=int, default=1,
                        help="Number of threads")

    args = parser.parse_args()

    if len(sys.argv[1:]) == 0:
        parser.print_help()
        sys.exit(1)

    Run(args)


if __name__ == "__main__":
    main()
