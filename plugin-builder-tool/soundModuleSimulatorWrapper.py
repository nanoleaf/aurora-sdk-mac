from printer import *
import os
import time
import glob
import threading
import subprocess

class SoundModuleSimulatorWrapper:

	def __init__(self, parent):
		self.parent = parent
		self.sms_thread = None
		self.sms_proc = None
		self.mp_thread = None
		self.mp_proc = None
		self.abort_signal = False
		self.exit_program = False
		self.ip_addr = ""
		self.plugin_path = ""
		self.palette_path = ""
		self.cwd = ""
		self.out_queue = None
		self.directory_divider = self.parent.directory_divider

	def run_music_processor(self):
		if self.mp_running():
			self.stop_music_proc()
		time.sleep(1)

		cwd = os.path.dirname(os.path.realpath(__file__)).split(self.directory_divider)
		cwd = cwd[:len(cwd) - 1]
		self.cwd = self.directory_divider.join(cwd) + self.directory_divider

		self.mp_thread = threading.Thread(target=self.sm_run)
		self.mp_thread.daemon = True
		self.mp_thread.start()

		iprint ("Music Processor thread launched")

	def sm_run(self):
		command = ["python", "../music_processor.py"]
		iprint(command)
		self.mp_proc = subprocess.Popen(command, cwd=os.path.dirname(os.path.realpath(__file__)), stderr=subprocess.PIPE, stdin=subprocess.PIPE)
		iprint("Subprocess created")
		while not self.exit_program:
			time.sleep(0.5)

		iprint("Music Processor thread quitting")

	def run_thread(self, ip, plugin, palette, _palette_entered):
		if self.is_running():
			self.stop()
		time.sleep(1)

		self.palette_path = palette
		self.palette_entered = _palette_entered
		self.abort_signal = False
		self.ip_addr = ip
		self.plugin_path = plugin

		cwd = os.path.dirname(os.path.realpath(__file__)).split(self.directory_divider)
		cwd = cwd[:len(cwd) - 1]
		self.cwd = self.directory_divider.join(cwd) + self.directory_divider

		self.sms_thread = threading.Thread(target=self.run)
		self.sms_thread.daemon = True
		self.sms_thread.start()

		iprint("Sound Module Simulator thread launched")

	def run(self):
		iprint("Starting up sound module simulator")
		upper_dir = os.path.dirname(os.path.realpath(__file__)) + "/../"
		lib_path = self.plugin_path + self.directory_divider + "Debug" + self.directory_divider
		os.chdir(lib_path)
		lib_name = ""
		for file in glob.glob("*.so"):
			lib_name = file
			break
		lib_path = lib_path + lib_name
		os.chdir(upper_dir)
		if self.palette_entered:
			command = ["./SoundModuleSimulator", "-i", str(self.ip_addr), "-p", lib_path, "-cp", self.palette_path, "1>$2"]
		else:
			command = ["./SoundModuleSimulator", "-i", str(self.ip_addr), "-p", lib_path, "1>$2"]
		iprint(command)
		try:
			self.sms_proc = subprocess.Popen(command, cwd=upper_dir, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
			iprint("Subprocess created")
		except OSError, oserr:
			if (str(oserr.errno) == "2"):
				iprint("Error: " + str(oserr.errno))
				iprint("Please make sure you have SoundModuleSimulator")
			else:
				iprint(oserr)
			return
		while not self.abort_signal:
			if (self.sms_proc.poll() != None):
				iprint("Process terminated, or plugin crashed!")
				self.abort_signal = True
				self.exit_program = True
			time.sleep(1)
		iprint("Exiting sound module simulator thread")
		self.abort_signal = False


	def stop(self):
		self.abort_signal = True
		if self.sms_proc != None:
			if self.sms_proc.poll() == None:
				self.sms_proc.kill()
			self.sms_proc = None
		if self.sms_thread != None:
			self.sms_thread.join()
			self.sms_thread = None

	def is_running(self):
		if self.sms_proc == None and self.sms_thread == None:
			return False
		return True

	def mp_running(self):
		if self.mp_proc == None and self.mp_thread == None:
			return False
		return True

	def stop_music_proc(self):
		self.exit_program = True
		if self.mp_proc != None:
			try:
				self.mp_proc.stdin.write('q')
			except IOError:
				pass
			self.mp_proc.kill()
			self.mp_proc = None
		if self.mp_thread != None:
			self.mp_thread.join()
			self.mp_thread = None


