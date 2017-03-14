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
import socket
import sys
import threading
import numpy as np
import librosa
import datetime as dt
from time import sleep


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
    def __init__(self, input_format):
        threading.Thread.__init__(self)
        self.input_format = input_format

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


def process_music_data(data_in, n_fft, n_output_bins, mag_scalar, mag_scalar_min):
    data_np = np.fromstring(data_in, 'Float32')

     # decimate data
    global sample_rate
    data_np = librosa.resample(data_np, sample_rate, sample_rate/2)

    # short time fft over n_fft samples
    fft_data = librosa.stft(data_np, n_fft,
                            hop_length=n_fft,
                            center=False)

    fft_data_mag = np.abs(fft_data) ** 2
    fft_data_mag = fft_data_mag[:, 0]

    # magnitude scaling
    mag_scalar = update_magnitude_scaling(fft_data_mag, mag_scalar, mag_scalar_min)
    fft_data_mag_norm = fft_data_mag / mag_scalar
    fft_output = (fft_data_mag_norm * (2 ** 7-1)).astype(np.uint8)
    fft_output = fft_output[0:n_output_bins]

    # energy
    energy_output = (fft_output.sum()).astype(np.uint16)
    return fft_output, energy_output


if __name__ == '__main__':

    # parameters
    input_format = pyaudio.paFloat32
    min_delay = 50

    n_fft = 2 ** 7
    n_bins_out = 32

    mag_scalar = 1.0
    mag_scalar_min = 0.1

    udp_host = "127.0.0.1"
    udp_port = 27182
    sound_feature_udp_port = 27184

    # start pyaudio thread
    pa_thread = PyAudioThread(input_format)
    pa_thread.start()
    sleep(1)

    # open udp socket to receive
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind((udp_host, sound_feature_udp_port))

    # receive sound feature
    packet, addr = udp_socket.recvfrom(20)  # blocking
    from_host = addr[0]
    udp_socket.close()

    # packet contains: [b i b] where b is boolean, i is integer
    if from_host == udp_host:
        tokens = packet.split()

    is_fft = int(tokens[0])
    n_bins_out = int(tokens[1])
    is_energy = int(tokens[2])
    print "sound feature: fft {} bins {} energy {}".format(is_fft, n_bins_out, is_energy)

    # open new udp socket to send
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    t_start = dt.datetime.now()

    # main loop
    data = []
    data_updated = False
    stop = False
    print "starting sound data transfer"

    # start pyaudio thread
    kp_thread = KeyPressThread()
    kp_thread.start()

    while True:

        pyaudio_lock.acquire()
        data_updated = data_buffer_updated
        data = data_buffer
        data_buffer_updated = False
        pyaudio_lock.release()

        # process and send music data
        if data_updated:

            (fft, energy) = process_music_data(data,
                                               n_fft, n_bins_out, mag_scalar, mag_scalar_min)

            t_end = dt.datetime.now()
            t_processing = (t_end - t_start).microseconds / 1e3
            t_sleep = min_delay - t_processing

            # print 'sleep for {:.2f} ms'.format(t_sleep)
            if t_sleep > 0.0:
                sleep(t_sleep/1e3)

            t_start = dt.datetime.now()

            if is_fft:
                message = fft.tobytes()
            else:
                message = (np.zeros(n_bins_out).astype(np.uint8)).tobytes()

            if is_energy:
                message += energy.tobytes()
            else:
                message += (np.zeros(2).astype(np.uint16)).tobytes()

            udp_socket.sendto(message, (udp_host, udp_port))

        # check for key press to quit loop
        keypress_lock.acquire()
        stop = stop_loop
        keypress_lock.release()

        if stop:
            print "stopping sound data transfer"
            break

    # stop pyaudio thread
    stop_pyaudio_thread = True
    pa_thread.join()

    # stop keypress thread
    kp_thread.join()



