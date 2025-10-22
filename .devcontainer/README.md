# Devcontainer

Here is contributed by @miltkall ([#22](https://github.com/PlotJuggler/plotjuggler-CAN-dbs/pull/22)) with the following description.

* Complete Development Container Environment
    * Docker-based Setup: Kasm Ubuntu 24.04 container with all CAN tools pre-installed
    * PlotJuggler Pre-built: Container includes PlotJuggler with CAN DBC plugin ready to use
    * VS Code Integration: Full devcontainer configuration with relevant extensions
    * Automated Setup: One-command development environment deployment
* CAN Simulation & Testing Tools
    * Python CAN Simulator: can_dbc_simulator.py generates realistic CAN traffic from DBC files
    * Enum-Aware Simulation: Simulator respects signal enum values and ranges defined in DBC
    * Multi-DBC Support: Can simulate multiple DBC files simultaneously on different interfaces
    * Automated Startup: Container automatically configures vcan0 and starts simulators

#### TODO:
* Scripts and configs will be reviewed and verified.