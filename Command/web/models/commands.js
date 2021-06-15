const mongoose = require("mongoose");
const Schema = mongoose.Schema;

const commandSchema = new Schema({
    time: {
        type: Number,
        required: true
    },
    mode: {
        type: String,
        required: true
    },
    amount: {
        type: Number,
        required: true
    }
}, { timestamps: true });

const Command = mongoose.model("Command", commandSchema);
module.exports = Command;