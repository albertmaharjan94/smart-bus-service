const mongoose = require('mongoose');
mongoose.connect("mongodb://127.0.0.1/smart_bus",{
  useNewUrlParser: true,
  useCreateIndex: true,
    useUnifiedTopology: true,
    server: { socketOptions: { connectTimeoutMS: 100000000 }}
});

