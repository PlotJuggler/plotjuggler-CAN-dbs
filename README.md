# plotjuggler-CAN-dbs
Plugins to visualize CAN .dbs files and streams in PlotJuggler.

Using the plugins, One can;
  * Visualize CAN logs created by `candump -L` in PlotJuggler using a can database file (for ex. .dbc)
  * Visualize CAN streams (yet to come)

# Prerequisites
  * Boost (1.72 or later)
  * libxml2

# Building the CAN plugins

```
mkdir build; cd build
cmake ..
make

```

# Using the plugins

After building the plugins, one needs to include build directory to the plugins directory of the PlotJuggler.

If DataLoadCAN plugin is loaded, you will be able to import `.log` files. Currently, no UI is developed, so in order to choose can database file (`.dbc`), another file dialog will be opened, select the database you want to use.

![DataLoadCAN](docs/DataLoadCAN.png "DataLoadCAN snapshot")
