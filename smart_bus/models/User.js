const mongoose = require('mongoose');
const Schema = mongoose.Schema;


const UserSchema = new Schema({
    fullname: {
        type: String,
        required: true
    },
    rfid: {
        type: String,
        required: true,
        unique: true,
    },
    pincode: {
        type: String,
        required: true
    },
    amount: {
        type: Number,
        required: true,
        default: 0
    }
}, { timestamps: true });

const User = mongoose.model('User', UserSchema);
module.exports = User;
