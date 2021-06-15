const http = require("http");
const fs = require("fs");
const url = require("url");
const cp = require("child_process");
const mongoose = require("mongoose");
const Command = require("./models/commands");
const { Parser } = require("json2csv");
const path = require("path");
const { kMaxLength } = require("buffer");

const dbURI = "mongodb+srv://walleee:walleee2021@wall-eee.yvkkh.mongodb.net/Wall-EEE?retryWrites=true&w=majority";
mongoose.connect(dbURI, { useNewUrlParser: true, useUnifiedTopology: true })
    .then((result) => console.log("Connected to database"))
    .catch((err) => console.log(err));

const host = '0.0.0.0';
const port = 8000;

const infoPeriod = 8000; //ms (i.e. 8 seconds)

//Current State
var state = {x: 0, y: 0, rotation: 0, speed: 0, charge_capacity: 100, charging_power: 0, range: 0};
var red = {seen: 0, x: 0, y: 0};
var blue = {seen: 0, x: 0, y: 0};
var green = {seen: 0, x: 0, y: 0};
var pink = {seen: 0, x: 0, y: 0};
var yellow = {seen: 0, x: 0, y: 0};

var map = {x: 0, y: 0, redx: 0, redy: 0, greenx: 0, greeny: 0, bluex: 0, bluey: 0, pinkx: 0, pinky: 0, yellowx: 0, yellowy: 0};

//reset the map
cp.exec("./update_map -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0 0 0 50",(err, stdout, stderr) => {
    //callback function
    //should update local variables here
    //for now:
    if(!err) console.log("Reset map");
});

function updateMap () {

    var max_coord = Math.max(state.x, state.y, red.x, red.y, blue.x, blue.y, green.x, green.y, pink.x, pink.y, yellow.x, yellow.y);
    var scale = Math.max(0.5, max_coord/1000);

    var args = "";

    if(red.seen) {
        args = Math.floor(red.x/scale) + " " + Math.floor(red.y/scale) + " ";
    } else {
        args = "-1 -1 ";
    }

    if(green.seen) {
        args += Math.floor(green.x/scale) + " " + Math.floor(green.y/scale) + " ";
    } else {
        args += "-1 -1 ";
    }

    if(blue.seen) {
        args += Math.floor(blue.x/scale) + " " + Math.floor(blue.y/scale) + " ";
    } else {
        args += "-1 -1 ";
    }

    if(pink.seen) {
        args += Math.floor(pink.x/scale) + " " + Math.floor(pink.y/scale) + " ";
    } else {
        args += "-1 -1 ";
    }

    if(yellow.seen) {
        args += Math.floor(yellow.x/scale) + " " + Math.floor(yellow.y/scale) + " ";
    } else {
        args += "-1 -1 ";
    }

    args += Math.floor(state.x/scale) + " " + Math.floor(state.y/scale) + " " + state.rotation + " ";

    args += Math.floor(100 * scale);

    cp.exec("./update_map " + args,(err, stdout, stderr) => {
        if(!err) console.log("Updated map");
        else console.log(err);
    });
};

function updateState(stdout){
    var updateMapNow = false;
    var seenCount = red.seen + green.seen + blue.seen + pink.seen + yellow.seen;

    var data = (stdout.trim().split(/[\n,]/)).map(Number);

    if(Math.abs(data[0]-map.x)>100 || Math.abs(data[1]-map.y)>100) updateMapNow = true;
    state.x = data[0];
    state.y = data[1];
    state.rotation = data[2];
    state.speed = data[4];
    state.charge_capacity = data[5];
    state.charging_power = data[6];
    state.range = data[7];

    var red_x_abs = state.x + data[10]*Math.sin(state.rotation * Math.PI / 180) + data[9]*Math.cos(state.rotation * Math.PI / 180); 
    var red_y_abs = state.y + data[10]*Math.cos(state.rotation * Math.PI / 180) - data[9]*Math.sin(state.rotation * Math.PI / 180); 

    if((Math.abs(red_x_abs-map.redx)>100 || Math.abs(red_y_abs-map.redy)>100) && data[8]) updateMapNow = true;

    if(data[8]){
        red.seen = 1;
        red.x = red_x_abs;
        red.y = red_y_abs;
    }

    var green_x_abs = state.x + data[13]*Math.sin(state.rotation * Math.PI / 180) + data[12]*Math.cos(state.rotation * Math.PI / 180); 
    var green_y_abs = state.y + data[13]*Math.cos(state.rotation * Math.PI / 180) - data[12]*Math.sin(state.rotation * Math.PI / 180); 

    
    if((Math.abs(green_x_abs-map.greenx)>100 || Math.abs(green_y_abs-map.greeny)>100) && data[11]) updateMapNow = true;

    if(data[11]){
        green.seen = 1;
        green.x = green_x_abs;
        green.y = green_y_abs;
    }

    var blue_x_abs = state.x + data[16]*Math.sin(state.rotation * Math.PI / 180) + data[15]*Math.cos(state.rotation * Math.PI / 180); 
    var blue_y_abs = state.y + data[16]*Math.cos(state.rotation * Math.PI / 180) - data[15]*Math.sin(state.rotation * Math.PI / 180); 

    if((Math.abs(blue_x_abs-map.bluex)>100 || Math.abs(blue_y_abs-map.bluey)>100) && data[14]) updateMapNow = true;

    if(data[14]){
        blue.seen = 1;
        blue.x = blue_x_abs;
        blue.y = blue_y_abs;
    }

    var pink_x_abs = state.x + data[19]*Math.sin(state.rotation * Math.PI / 180) + data[18]*Math.cos(state.rotation * Math.PI / 180); 
    var pink_y_abs = state.y + data[19]*Math.cos(state.rotation * Math.PI / 180) - data[18]*Math.sin(state.rotation * Math.PI / 180); 
    
    if((Math.abs(pink_x_abs-map.pinkx)>100 || Math.abs(pink_y_abs-map.pinky)>100) && data[17]) updateMapNow = true;

    if(data[17]){
        pink.seen = 1;
        pink.x = pink_x_abs;
        pink.y = pink_y_abs;
    }

    var yellow_x_abs = state.x + data[22]*Math.sin(state.rotation * Math.PI / 180) + data[21]*Math.cos(state.rotation * Math.PI / 180); 
    var yellow_y_abs = state.y + data[22]*Math.cos(state.rotation * Math.PI / 180) - data[21]*Math.sin(state.rotation * Math.PI / 180); 

    if((Math.abs(yellow_x_abs-map.yellowx)>100 || Math.abs(yellow_y_abs-map.yellowy)>100) && data[20]) updateMapNow = true;

    if(data[20]){
        yellow.seen = 1;
        yellow.x = yellow_x_abs;
        yellow.y = yellow_y_abs;
    }

    var newSeenCount = red.seen + green.seen + blue.seen + pink.seen + yellow.seen;
    if(seenCount != newSeenCount) updateMapNow = true;

    if(updateMapNow){
        updateMap();
        map.x = state.x;
        map.y = state.y;
        map.redx = red.x;
        map.redy = red.y;
        map.greenx = green.x;
        map.greeny = green.y;
        map.bluex = blue.x;
        map.bluey = blue.y;
        map.pinkx = pink.x;
        map.pinky = pink.y;
        map.yellowx = yellow.x;
        map.yellowy = yellow.y;
    }
};


const requestListener = function (req, res) {
    let parsedURL = url.parse(req.url,true);
    let path = parsedURL.path.replace(/^\/+|\/+$/g, "");

    if (path == ""){
        path = "index.html";
    }

    if(path.includes("images/map.jpg")){
        path = "images/map.jpg";
    }
    
    if (path == "command"){

        let data = "";
        var obj;

        req.on('data', (chunk) => {
            data+= chunk;
        });

        req.on('end', () => {
            obj = JSON.parse(data);
            console.log('Received command ' + obj.mode + ' ' + obj.amount);
            //send command to tcp program
            cp.exec("./tcp_client "+obj.mode+" "+obj.amount,(err, stdout, stderr) => {
                if(!err){
                    console.log("Sent command");
                    updateState(stdout);
                } else {
                    console.log(err);
                }
                
            });
            //end of tcp section

            res.writeHead(200);
            res.end();

            //adding command to database
            const curCommand = new Command({
                time: obj.time,
                mode: obj.mode,
                amount: obj.amount
            });
            curCommand.save()
                .then((result) => {
                    console.log("Updated remote database");
                })
                .catch((err) => {
                    console.log(err);
                });
        });

    } else if (path == "update"){

        res.setHeader("X-Content-Type-Options", "nosniff");
        res.writeHead(200, {"Content-type": "application/json"});
        res.end(JSON.stringify(state));
        console.log("Returned current state");

    } else {
        console.log('Requested path ' + path);
        let file = __dirname + "/public/" + path;

        if(path == "database.csv"){
            Command.find()
                .then((result) => {
                    const fields = ["_id", "time", "mode", "amount", "createdAt", "updatedAt"];
                    const fieldNames = ["ID", "Time", "Mode", "Amount", "CreatedAt", "UpdatedAt"];
                    const json2csvParser = new Parser({fields: fields, fieldNames: fieldNames});
                    const csvData = json2csvParser.parse(result);

                    fs.writeFile("./public/database.csv", csvData, function(error) {
                        if (error) throw error;
                        else {
                            console.log("Updated local database");
                            fs.readFile(file, (err, content) => {
                                if(err) {
                                    console.log('File Not Found ' + file);
                                    res.writeHead(404);
                                    res.end();
                                } else {
                                    console.log('Returning ' + path);
                                    res.setHeader("X-Content-Type-Options", "nosniff");
                                    res.writeHead(200, {"Content-type": "text/csv"});
                                    res.end(content);
                                }
                            });
                        }
                        
                    });
                })
                .catch((err) => {
                    console.log(err);
                });
        } else {
            fs.readFile(file, (err, content) => {
                if(err) {
                    console.log('File Not Found ' + file);
                    res.writeHead(404);
                    res.end();
                } else {
                    console.log('Returning ' + path);
                    res.setHeader("X-Content-Type-Options", "nosniff");
                    switch (path) {
                        case "index.html":
                            res.writeHead(200, {"Content-type": "text/html"});
                            break;
                        case "style.css":
                            res.writeHead(200, {"Content-type": "text/css"});
                            break;
                        case "main.js":
                            res.writeHead(200, {"Content-type": "application/javascript"});
                            break;
                        case "background.jpg" || "map.jpg":
                            res.writeHead(200, {"Content-type": "image/jpg"});
                            break;
                        case "myfavicon.ico":
                            res.writehead(200, {"Content-type": "image/x-icon"});
                    }
                    res.end(content);
                }
            });
        }
    }
};

const server = http.createServer(requestListener);
server.listen(port, host, () => {
    console.log('Server is running on http://'+host+':'+port);
});

function getInfo() {
    cp.exec("./tcp_client info",(err, stdout, stderr) => {
        if(!err){
            console.log("Sent req for info");
            updateState(stdout);    
        } else {
            console.log(err);
        }
    });
};

setInterval(getInfo, infoPeriod);