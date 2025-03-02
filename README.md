# PROVE Lab - Mila (Electric Endurance Vehicle)
![Formatting Workflow](https://github.com/PROVELab/ProveMila-embedded/actions/workflows/check-format/badge.svg)

Mila is an electric sports car with the goal of going 1000 miles on a single charge. This is the GitHub repository for the Low Voltage system. It's built on [PlatformIO](https://platformio.org/).

## Project Structure
The `src` folder contains the source code for all the embedded components of the system. Inside the `src` folder, there are folders for each node of the system and a `common` folder for shared code and drivers. See the READMEs within each folder for further information about each node.

## Formatting
To format all c/cpp files in the repo (src and include), run:
```
./format.sh
```
This will not affect lib (since it's external libraries)
