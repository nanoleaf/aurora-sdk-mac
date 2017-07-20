# Copyright 2017 Nanoleaf Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import pyaudio
import librosa
import numpy as np
import argparse
import socket
import sys
import threading
from time import sleep, time
from distutils.version import StrictVersion


pyaudio_lock = threading.Lock()
keypress_lock = threading.Lock()
stop_pyaudio_thread = False
data_buffer = []
data_buffer_updated = False
sample_rate = 0
stop_loop = False


class KeyPressThread (threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

    def run(self):
        global stop_loop
        while True:
            key = raw_input("enter q to quit\n>>> ")
            if key == "q":
                keypress_lock.acquire()
                stop_loop = True
                keypress_lock.release()
                break
            sleep(0.2)


class PyAudioThread (threading.Thread):
    def __init__(self, input_samples, input_format):
        threading.Thread.__init__(self)
        self.input_format = input_format
        self.input_samples = input_samples

    def run(self):
        # create pyaudio object
        pa = pyaudio.PyAudio()

        # get default host/input device info dicts
        try:
            host_api_info = pa.get_default_host_api_info()
            default_input_device_info = pa.get_default_input_device_info()
        except IOError:
            print("Input audio device not found, terminating ...")
            pa.terminate()
            sys.exit()

        default_input_sample_rate = default_input_device_info['defaultSampleRate']          # float
        default_low_input_latency = default_input_device_info['defaultLowInputLatency']     # float
        default_high_input_latency = default_input_device_info['defaultHighInputLatency']   # float
        default_max_input_channels = default_input_device_info['maxInputChannels']          # integer

        print "default inputs: sample rate {}, latency low {:.4f}, latency high {:.4f}, channels {}".\
            format(default_input_sample_rate, default_low_input_latency, default_high_input_latency, default_max_input_channels)

        global sample_rate
        pyaudio_lock.acquire()
        sample_rate = default_input_sample_rate
        pyaudio_lock.release()

        stream = pa.open(rate=int(sample_rate),
                         channels=1,
                         format=self.input_format,
                         input=True,
                         frames_per_buffer=self.input_samples,
                         stream_callback=PyAudioThread.input_callback)

        stream.start_stream()
        global stop_pyaudio_thread
        while not stop_pyaudio_thread:
            sleep(0.1)

        stream.stop_stream()
        stream.close()

        # terminate pyaudio object
        pa.terminate()

    @staticmethod
    def input_callback(in_data, frame_count, time_info, status):
        global data_buffer, data_buffer_updated
        pyaudio_lock.acquire()
        data_buffer = in_data       # fill data
        data_buffer_updated = True  # set updated flag
        pyaudio_lock.release()
        return None, pyaudio.paContinue


def update_magnitude_scaling(mag, scalar, min_scalar):
    '''

    :param mag:     numpy array of magnitudes of frequency bins
    :param scalar:      previous scalar
    :param min_scalar:  minimum scalar
    :return:            updated scalar
    '''
    max_mag = np.max(mag)

    mag_diff = max_mag - scalar
    updated_scalar = scalar + 0.02 * mag_diff

    if scalar < min_scalar:
        updated_scalar = min_scalar

    # print "{} ({})".format(updated_scalar, max_mag)
    return updated_scalar


def visualizer(data_in):
    scalar = 5
    hi_limit = 100
    value = np.sum(np.abs(data_in) ** 2) * scalar

    if value > hi_limit:
        value = hi_limit

    value = int(value)

    # back to front of row
    sys.stdout.write('\r')

    # print | followed by spaces
    for i in range(0, value, 1):
        sys.stdout.write('|')
    for i in range(value, hi_limit, 1):
        sys.stdout.write(' ')
    sys.stdout.write('\n')

    # flush output
    sys.stdout.flush()


def check_min_versions():
    ret = True

    # pyaudio
    vers_required = "0.2.7"
    vers_current = pyaudio.__version__
    if StrictVersion(vers_current) < StrictVersion(vers_required):
        print "Error: minimum pyaudio vers: {}, current vers {}".format(vers_required, vers_current)
        ret = False

    # librosa
    vers_required = "0.4.3"
    vers_current = librosa.__version__
    if StrictVersion(vers_current) < StrictVersion(vers_required):
        print "Error: minimum librosa vers: {}, current vers {}".format(vers_required, vers_current)
        ret = False

    # numpy
    vers_required = "1.9.0"
    vers_current = np.__version__
    if StrictVersion(vers_current) < StrictVersion(vers_required):
        print "Error: minimum numpy vers: {}, current vers {}".format(vers_required, vers_current)
        ret = False

    return ret


def get_output_fft_bins(fft_mag, n_out):
    n_in = len(fft_mag)
    step_size = int(n_in/n_out)
    fft_out = np.zeros(n_out)
    n_filled = 0
    i = 0
    while n_filled < n_out:
        acc = np.sum(fft_mag[i:min(i+step_size, n_in)])
        acc /= step_size
        i += step_size
        # saturate to 8-bit unsigned
        if acc > 255:
            acc = 255
        fft_out[n_filled] = acc
        n_filled += 1
    return fft_out[0:n_out]


def process_music_data(data_in, is_fft, is_energy, n_output_bins, n_fft, is_visual):
    # length is len(data_in)/4
    data_np = np.fromstring(data_in, 'Float32')

    # visualizer
    if is_visual:
        visualizer(data_np)

    # energy
    if is_energy:
        energy = np.abs(data_np) ** 2
        energy = energy.sum()
        energy *= 2**5
        energy_output = energy.astype(np.uint16)
    else:
        energy_output = np.zeros(2).astype(np.uint16)

    # fft
    if is_fft:
        global sample_rate

        # down-sample by 4, with filtering, energy not scaled
        data_np = librosa.resample(data_np,
                                   sample_rate,
                                   sample_rate/4,
                                   res_type='kaiser_fast')

        # short time fft over n_fft samples
        fft_data = librosa.stft(data_np, n_fft,
                                hop_length=n_fft,
                                center=False)

        fft_data_mag = np.abs(fft_data[0:n_fft/2]) ** 2

        # magnitude scaling
        fft_data_mag *= 2**3
        fft_output = get_output_fft_bins(fft_data_mag, n_output_bins)
        fft_output = fft_output.astype(np.uint8)
    else:
        fft_output = np.zeros(n_output_bins).astype(np.uint8)

    return fft_output, energy_output


if __name__ == '__main__':

    # parameters
    input_samples = 2**11
    input_format = pyaudio.paFloat32
    min_delay = 50

    n_fft = 512

    udp_host = "127.0.0.1"
    udp_port = 27182
    sound_feature_udp_port = 27184

    # check minimum versions of imported modules
    if not check_min_versions():
        print "Minimum version not satisfied, please upgrade your modules as indicated!"
        exit(1)

    # parse command arguments
    parser = argparse.ArgumentParser(description="Music processing and streaming script for the Nanoleaf Rhythm SDK")
    parser.add_argument("--viz", help="turn on simple visualizer, please limit use to setup and debug", action="store_true")
    args = parser.parse_args()
    visualize = args.viz

    # open udp socket to receive
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind((udp_host, sound_feature_udp_port))

    # prompt user to start plugin
    print "Music processor initialized... please run your plugin to continue or ctrl+c to exit"

    # receive sound feature
    packet, addr = udp_socket.recvfrom(20)  # blocking, ctrl+c to exit
    from_host = addr[0]
    udp_socket.close()
    print "Plugin detected... continuing"

    # packet contains: [b i b] where b is boolean, i is integer
    if from_host == udp_host:
        tokens = packet.split()

    is_fft = int(tokens[0])
    n_bins_out = int(tokens[1])
    is_energy = int(tokens[2])
    # print "Sound features requested: fft {} fft bins {} energy {}".format(is_fft, n_bins_out, is_energy)

    # start pyaudio thread
    pa_thread = PyAudioThread(input_samples, input_format)
    pa_thread.start()
    sleep(1)

    # open new udp socket to send
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # main loop
    data = []
    data_updated = False
    stop = False
    print "Music processor active!"
    if visualize:
        print "Visualize on: try a loud clap and a simple sound bar should appear"
    else:
        print "If nothing seems to be happening, try running with --viz"

    # start key press thread
    kp_thread = KeyPressThread()
    kp_thread.start()

    # start timer
    startTime = time()

    # main processing loop
    while True:
        pyaudio_lock.acquire()
        data_updated = data_buffer_updated
        data = data_buffer
        data_buffer_updated = False
        pyaudio_lock.release()

        # process and send music data
        if data_updated:
            (fft, energy) = process_music_data(data,
                                               is_fft,
                                               is_energy,
                                               n_bins_out,
                                               n_fft,
                                               visualize)

            stopTime = time()
            elapsedTime = (stopTime - startTime) * 1000
            sleepTime = min_delay - elapsedTime
            # print 'buffer + process time {:.2f} ms, sleep for {:.2f} ms'.format(elapsedTime, sleepTime)
            if sleepTime > 0.0:
                sleep(sleepTime/1e3)

            # message to simulator
            message = fft.tobytes() + energy.tobytes()
            # print "fft {} energy {}".format(fft, energy)
            
            udp_socket.sendto(message, (udp_host, udp_port))

            startTime = time()

        # check for key press to quit loop
        keypress_lock.acquire()
        stop = stop_loop
        keypress_lock.release()

        if stop:
            print "Stopping music processor!"
            break

    # stop pyaudio thread
    stop_pyaudio_thread = True
    pa_thread.join()

    # stop keypress thread
    kp_thread.join()

