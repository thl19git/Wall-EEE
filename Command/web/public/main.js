var path = document.referrer;

var commands = ["","","","",""];

function logUpdate(commands, mode, amount){
    var command;
    if(mode=="move"){
        if(amount>=0){
            command = "- Move forwards " + Math.abs(amount) + "mm";
        } else {
            command = "- Move backwards " + Math.abs(amount) + "mm";
        }
    } else {
        if(amount>=0){
            command = "- Turn right " + Math.abs(amount) + " degrees";
        } else {
            command = "- Turn left " + Math.abs(amount) + " degrees";
        }
    }

    var full = true;
    for(var i = 0; i < commands.length; i++){
        if(commands[i]==""){
            commands[i] = command;
            full = false;
            break;
        }
    }
    if(full){
        document.getElementById('recentCommands').style.display = "inline-block";
        document.getElementById('recentCommands').style.lineHeight = 1.6;
        document.getElementById('recentCommands').style.marginBottom = "1em";
        document.getElementById('commandLogH').style.lineHeight = 1;
        document.getElementById('commandLogH').style.marginBottom = 0;
        for(var i = 0; i < commands.length-1; i++){
            commands[i] = commands[i+1];
        }
        commands[4] = command;
    }
    document.getElementById("cl1").innerHTML = commands[0];
    document.getElementById("cl2").innerHTML = commands[1];
    document.getElementById("cl3").innerHTML = commands[2];
    document.getElementById("cl4").innerHTML = commands[3];
    document.getElementById("cl5").innerHTML = commands[4];
}

document.getElementById("move").onclick = function() {
    document.getElementById("units").innerHTML = "mm";
    document.getElementById("amount").min = "-10000";
    document.getElementById("amount").max = "10000";
    document.getElementById("amount").step = "1";
};

document.getElementById("rotate").onclick = function() {
    document.getElementById("units").innerHTML = "deg";
    document.getElementById("amount").min = "-180";
    document.getElementById("amount").max = "180";
    document.getElementById("amount").step = "0.1";
};

document.getElementById("CommandForm").onsubmit = function(ev) {
    ev.preventDefault();
    var mode_ = document.querySelector('input[name="mode"]:checked').value;
    var amount_ = document.getElementById("amount").value;
    fetch(path+"/command", {
        method: 'POST',
        body: JSON.stringify({
            time: Date.now(),
            mode: mode_,
            amount: amount_
        }),
        headers: {
            'Content-type': 'application/json; charset=UTF-8'
        }
    });
    document.getElementById("CommandForm").reset();
    document.getElementById("units").innerHTML = "mm";
    document.getElementById("amount").min = "-10000";
    document.getElementById("amount").max = "10000";
    document.getElementById("amount").step = "1";
    logUpdate(commands, mode_, amount_);
};

function refresh() {
    //refresh the status info
    fetch(path+"/update")
        .then(response => response.json())
        .then(data => {
            document.getElementById("batteryLevel").innerText = "Battery Level: " + data.charge_capacity + "%";
            document.getElementById("location").innerText = "Location: (" + data.x + "," + data.y + ")";
            document.getElementById("speed").innerText = "Speed: " + data.speed + " mm/s";
            document.getElementById("rotation").innerText = "Rotation: " + data.rotation + " degrees";
            document.getElementById("chargePower").innerText = "Charging Power: " + data.charging_power + "%";
            document.getElementById("range").innerText = "Range: " + data.range +  " mm";
        });

    //refresh the map
    var timestamp = new Date().getTime();
    document.getElementById("map").src = "images/map.jpg?t=" + timestamp;
};

setInterval(refresh, 5000);