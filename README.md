# PROVE Lab - Mila (Electric Endurance Vehicle)
Mila is an electric sports car with the goal of going 1000 miles on a single charge. This is the GitHub repository for the Low Voltage system. It's built on [PlatformIO](https://platformio.org/).

## Project Structure
The `src` folder contains the source code for all the embedded components of the system. Inside the `src` folder, there are folders for each node of the system and a `common` folder for shared code and drivers. See the READMEs within each folder for further information about each node.

## Setup (Flatbuffers)
If working with the Dashboard <-> MCU connection, you'll want to get a good understanding of 
Flatbuffers. Reference the flatbuffer documentation online to understand how it works, but tldr
it generates code based off of a schema, allowing a plain schema file to define how
communication is defined and serialized.
