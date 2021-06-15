# Command

## Dependencies

For the TCP client the only dependancy is g++.
On linux this can be installed using `sudo apt-get install g++`.

For the map generation the only dependancy is ImageMagick.
On linux this can be installed using `sudo apt-get install imagemagick`.

For the web server there are three main dependancies, Node.js and two packages, mongoose and json2csv.
To install Node.js and npm (node package manager) on linux run the following:

    $ curl -sL https://deb.nodesource.com/setup_12.x | sudo -E bash -
    $ sudo apt-get install -y nodejs
    
Check it has been installed by running `node -v`

To install the necessary packages run the following:

    $ npm install mongoose
    $ npm install json2csv
    
## Usage

All important code is in the web directory, so do `cd web`.

Compile the TCP client: `g++ tcp_client.cpp -o tcp_client`.

Compile the map updater: `g++ update_map.cpp -o update_map`.

To run the web server run `node server.js`.
The control centre can then be accessed from localhost:8000.

If you wish to run the test server, first compile `data.cpp` in the data directory: `g++ data.cpp -o data`.
From the web directory you can then run `node test_server.js`. This works the same way as the regular server but uses randomly generated data rather than data from the rover.

To bypass the web server and send commands directly from the command line, execute tcp_client with two input arguments, mode of movement and amount.

e.g. to move the rover forward 100mm run `./tcp_client move 100`

e.g. to rotate the rover 90 degrees left run `./tcp_client rotate -90`
