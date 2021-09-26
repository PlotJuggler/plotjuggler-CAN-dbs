# plotjuggler-CAN-dbs
Plugins to visualize CAN .dbs files and streams in PlotJuggler.

Using the plugins, One can;
  * Visualize CAN logs created by `candump -L` in PlotJuggler using a can database file (for ex. .dbc)
  * Visualize CAN streams (yet to come)

# Prerequisites
  * Boost (1.72 or later)
  * libxml2
  * PlotJuggler (3.2 or later)
  * Qt5 SerialBus (Optional, required for DataStreamCAN)

# Building the CAN plugins

```
mkdir build; cd build
cmake ..
make

```

# Using the plugins

After building the plugins, one needs to include build directory to the plugins directory of the PlotJuggler.

## DataLoadCAN

If DataLoadCAN plugin is loaded, you will be able to import `.log` files. Currently, no UI is developed, so in order to choose can database file (`.dbc`), another file dialog will be opened, select the database you want to use.

![DataLoadCAN](docs/DataLoadCAN.png "DataLoadCAN snapshot")

## DataStreamCAN

CAN Streamer plugin is only built if Qt5 SerialBus plugin is installed in your machine (surprisingly, it is not possible to install via apt on Ubuntu 18.04).
If you want to use CAN Streamer plugin (and your machine does not have the plugin), you can check [this gist](https://gist.github.com/awesomebytes/ed90785324757b03c8f01e3ffa36d436) for instructions on how to install Qt5 Serialbus.

When you start CAN Streamer plugin, a connect dialog will be opened as in the figures below. After choosing the correct backend and interface, one need to load CAN Database (`.dbc` only for now) to be able to start the streamer (OK button is disabled by default, it is enabled only when a database is loaded.).

![DataStreamCAN](docs/DatabaseLoaded.png "DataStreamCAN connect, database loaded.")
