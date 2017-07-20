# Installation Instructions
The [music_processor](./music_processor.py) python script requires the latest version of the following (non-standard) python packages

- [numpy](http://www.numpy.org/)
- [pyaudio](https://people.csail.mit.edu/hubert/pyaudio/)
- [librosa](https://github.com/librosa/librosa)

The recommended installation method is _pip_. See [here](http://pip.readthedocs.io/en/stable/installing/) for _pip_ installation instructions. Clone or download the appropriate SDK repo from our [GitHub Page](https://github.com/nanoleaf). 

The following installation requires _homebrew_. See [here](https://brew.sh/) for installation instructions. 
```
brew install portaudio
```

Start and activate a new Python (2.7.10) [virtualenv](http://python-guide-pt-br.readthedocs.io/en/latest/dev/virtualenvs/)
```
virtualenv venv
source venv/bin/activate
```

Install the following dependencies using _pip_
```
pip install numpy
pip install --global-option='build_ext' --global-option='-I/usr/local/include' --global-option='-L/usr/local/lib' pyaudio
pip install librosa
```

# Testing Your Plugin
## Compile Your Plugin
The _AuroraPlugin_ Framework comes with a makefile and a utilities library.
Once you have completed the implementation of your plugin, you can build it using the makefile in the Debug folder. If any additional libraries have been used, the makefile must be modified to link those libraries in as well during linking.

To compile, use a terminal and change your working directory to the PluginSDK/Debug folder. Use the following commands:
```
make clean
make all
```

Once the compilation completes successfully, a **libAuroraPlugin.so** file will be placed in the Debug folder which can be used with the simulator

## Run Your Plugin

Before running the simulator, a symbolic link will have to be made in `/usr/local/lib` to the libPluginUtilities.so file that is stored in the utilities folder of the AuroraPlugin directory.

To make this link, type the following into terminal.

`ln -s <Path>/Utilities/libPluginUtilties.so /usr/local/lib/libPluginUtilities.so`

To run the simulator, open up a terminal and change your working directory to the PluginSDK folder.
The music_processor python script must be run first, by entering:

`python music_processor.py`

Next, to run the simulator, enter:

`./SoundModuleSimulator -p <absolute path to .so file> -i <ip address>`


The *.so* file is the compiled plugin that you wish to run on the Aurora. 

The IP address that you enter is the ip address of the Aurora on the local network. The ip address can be found by using [Bonjour Browser](http://www.tildesoft.com).

When using the simulator for the first time, the simulator will attempt to acquire an authentication token from the Aurora and ask the user to hold down the power button on the Aurora for 5-7 seconds. This step is not required during subsequent executions of the simulator. 

Note: the Simulator will only maintain authentication with one Aurora at a time.
