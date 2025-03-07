// Complete Project Details at: https://RandomNerdTutorials.com/

// Database Paths
var dataFloatPath = 'test/float';
var dataIntPath = 'test/int';
var dataFloatPath1 = 'test/float1';

// Get a database reference 
const databaseFloat = database.ref(dataFloatPath);
const databaseInt = database.ref(dataIntPath);
const databaseFloat1 = database.ref(dataFloatPath1);

// Variables to save database current values
var floatReading;
var intReading;
var floatReading1;

// Attach an asynchronous callback to read the data
databaseFloat.on('value', (snapshot) => {
  floatReading = snapshot.val();
  console.log(floatReading);
  document.getElementById("reading-float").innerHTML = floatReading;
}, (errorObject) => {
  console.log('The read failed: ' + errorObject.name);
});

databaseInt.on('value', (snapshot) => {
  intReading = snapshot.val();
  console.log(intReading);
  document.getElementById("reading-int").innerHTML = intReading;
}, (errorObject) => {
  console.log('The read failed: ' + errorObject.name);
});

databaseFloat1.on('value', (snapshot) => {
    floatReading1 = snapshot.val();
    console.log(floatReading1);
    document.getElementById("reading-float1").innerHTML = floatReading1;
  }, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
  });

  // Dapatkan data dari Firebase
database.ref('test/sensors').on('value', function(snapshot) {
    const data = snapshot.val();
    displayData(data);
});

// Fungsi untuk menampilkan data
function displayData(data) {
    const container = document.getElementById('sensorData');
    container.innerHTML = ''; // Bersihkan konten sebelumnya
    data.forEach((item, index) => {
        const elem = document.createElement('div');
        elem.innerHTML = `<h2>Sensor ${index + 1}: ${item.sensor}</h2>
                          <p>Value: ${item.value}</p>
                          <p>Category: ${item.category}</p>`;
        container.appendChild(elem);
    });
}