require('./configurations/setup')

const express = require("express")
const app = express();
const bodyParser = require('body-parser');
const cors = require('cors');
const fs = require('fs');
const User = require('./models/User');
const morgan = require('morgan');

app.use(cors());
app.use(morgan("dev"))

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
    extended : false,
}));

app.get('/admin/users', async(req, res)=>{
    await User.find().then((data)=>{
        res.status(200).json(data)
    }).catch((err)=>{
        res.status(500).json({"success": false})
    })
})

app.post('/admin/create_user/:auth', async (req, res)=>{
    if(req.params.auth != "42069")
        res.status(403).json({"success": false, "message": "Unauthorized"});
    else{
        try{
            await User.findOne({rfid: req.body.rfid}).then((data)=>{
                if(data){
                    return res.status(500).json({"success": false, "msg": "RFID already exists"});
                }
            });
                
            user = User({
                fullname: req.body.fullname,
                rfid: req.body.rfid,
                pincode: req.body.pincode,
                amount: req.body.amount,
            })
            
            var validate = user.validateSync()
            
            if(!!validate) { 
                return res.status(500).json({"success": false, "msg": validate.message});
            }
            await user.save()
            return res.status(200).json({
                success: true,
                message: 'User Created',
                name: user.fullname,
                rfid: user.rfid,
                pincode: user.pincode,
                amount: user.amount,
            });
        }catch(e){
            console.log(e)
            res.status(500).json({"success": false, "message": "Server Error"})
        }
    }
});

app.put('/admin/update_amount/:auth/', async (req, res)=>{
    if(req.params.auth != "42069")
        res.status(403).json({"success": false, "message": "Unauthorized"});
    else{
        try{
            await User.findOne({_id: req.body._id}).then(async (data)=>{
                if(data){
                    data.amount += req.body.amount;
                    await data.save()
                    res.status(200).json({"success": false, "message": "Amount Added", "data": `User: ${data._id} \nAmount: ${data.amount}`});
                }
                else
                    res.status(201).json({"success": false, "message": "No User found"});
            })
            
        }catch(e){
            console.log(e)
            res.status(500).json({"success": false, "message": "Server Error"})
        }
    }
});



app.get('/api/user/:rfid/:pincode/:amount', async (req, res)=>{
    console.log("rfid and pincode")
    try{
        user = await User.findOne({rfid: req.params.rfid}).exec()
        if(user){
            console.log(user.pincode, req.params.pincode);
            if(user.pincode == req.params.pincode){
                if(user.amount >= req.params.amount){
                    user.amount -= req.params.amount
                    await user.save()
                    console.log(user._id);
                    res.status(200).send("#1,1,"+ user.amount);
                }else
                    res.status(201).send("#1,0,"+"Insufficient Amount")
            }
            else
                res.status(201).send("#1,0,"+"Incorrect Pin")
        }else
            res.status(201).send("#1,0,"+"User Not Found")
    }catch(e){
        console.log(e)
        res.status(201).send("#1,0,"+"Server Error")
    }
})


app.get('/api/user/:rfid', async (req, res)=>{
    try{
        user = await User.findOne({rfid: req.params.rfid}).exec()
        if(user)
            res.status(200).send("#0,1,"+ user.amount);
        else
            res.status(201).send("#0,0,"+"User Not found.")
    }catch(e){
        console.log(e)
        res.status(500).send("#0,0,"+"Server error.")
    }
})

    
app.listen(process.env.PORT||5000,() => {
    console.log("Server is listening to port 5000")
})


