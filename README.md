# Installation Instructions
The *music_processor* python script requires the latest version of the following (non-standard) python packages

* _numpy_
* _pyaudio_
* _librosa_

The recommended installation method is _pip_. See [here](http://pip.readthedocs.io/en/stable/installing/) for _pip_ installation instructions.

Clone or download the appropriate SDK repo from our [GitHub Page](https://github.com/nanoleaf)

## Windows
To use the SDK on Windows, Cygwin needs to be installed. For installation instructions, see [here](https://cygwin.com/install.html )

Install miniconda for Windows from [here](https://conda.io/miniconda.html). Assume the absolute path to its root directory is *CONDA_ROOT*. In Cygwin, miniconda is called by the _conda_ command.

In Cygwin, install up-to-date versions of _make_ and _gcc_

The following is executed within Cygwin:

Append _CONDA_ROOT_ to _PATH_ by 
`export PATH=CONDA_ROOT:$PATH`

Check that the python called by the shell is the one in `CONDA_ROOT` by `which python`. Verify the output is `CONDA_ROOT/python`

Install numpy by
`conda install -c anaconda numpy`

Install librosa by
`conda install -c conda-forge librosa`

Note that _pyaudio_ cannot be installed using _conda_ on Windows. Use the _pip_ packaged with _conda_ by:

`CONDA_ROOT/Scripts/pip install pyaudio`

verify that pyaudio has been installed for conda by 
_conda list_

## macOS
The following installation requires _homebrew_. See [here](https://brew.sh/) for installation instructions. 

`pip install numpy`

`brew install portaudio`

`pip install pyaudio`

`pip install librosa`

## Linux
For successful pyaudio install, make sure the packages *portaudio19-dev* and *python-dev-all* have been installed for your linux distribution, e.g., by using 

`sudo apt-get install portaudio19-dev`

`pip install numpy`

`pip install pyaudio`

`pip install librosa`

# Testing Your Plugin
## Compile Your Plugin
The _AuroraPlugin_ Framework comes with a makefile and a utilities library.
Once you have completed the implementation of your plugin, you can build it using the makefile in the Debug folder. If any additional libraries have been used, the makefile must be modified to link those libraries in as well during linking.

To compile, use a terminal window (Cygwin terminal on windows) and change your working directory to the PluginSDK/Debug folder. Use the following commands:

`make clean`

`make all`

Once the compilation completes successfully, a **libAuroraPlugin.so** file will be placed in the Debug folder which can be used with the simulator
## Run Your Plugin

On macOS and Linux, before running the simulator, a symbolic link will have to be made in `/usr/local/lib` to the libPluginUtilities.so file that is stored in the utilities folder of the AuroraPlugin directory.
To make this link, type the following into terminal.

macOS:
`ln -s <Path>/Utilities/libPluginUtilties.so /usr/local/lib/libPluginUtilities.so`

Ubuntu:
`ln -s <Path>/Utilities/libPluginUtilties.so /usr/lib/libPluginUtilities.so`

where _Path_ is the absolute path to the Aurora Plugin directory on your computer. No link is required in Cygwin, since the utilities library is statically linked in windows.

To run the simulator, open up a terminal (Cygwin terminal on windows) and change your working directory to the PluginSDK folder.
The music_processor python script must be run first, by entering:

`python music_processor.py`

Next, to run the simulator, enter:

`./SoundModuleSimulator -p <absolute path to .so file> -i <ip address>`


The *.so* file is the compiled plugin that you wish to run on the Aurora. 

The IP address that you enter is the ip address of the Aurora on the local network. The ip address can be found by using [Bonjour Browser](http://www.tildesoft.com) on macOS and [SSDP Scanner](https://www.microsoft.com/en-us/store/p/ssdp-scanner/9wzdncrcs2n7 ) on Windows.

When using the simulator for the first time, the simulator will attempt to acquire an authentication token from the Aurora and ask the user to hold down the power button on the Aurora for 5-7 seconds. This step is not required during subsequent executions of the simulator. Note: the Simulator will only maintain authentication with one Aurora at a time.