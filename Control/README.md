# Control

The purpose of the control sub-system is to act as the central component which relays information to and from the relevant parts of the rover. The ESP32 microcontroller is what was used to receive and communicate the information to the other sub-systems. This sub-system had 5 main tasks: 
* Receive information from the Vision sub-system 
* Receive information from the Drive sub-system 
* Receive information from the Energy sub-system 
* Receive commands from the Command sub-system and communicate this to the Drive and Energy sub-systems 
* Send all the information received to the Command sub-system 

Command would send an instruction to the Rover via Control. This instruction would then be relayed to Drive and Energy for them to perform the tasks. They, along with Vision, will then send back some information to Control which in turn is relayed to Command. 

