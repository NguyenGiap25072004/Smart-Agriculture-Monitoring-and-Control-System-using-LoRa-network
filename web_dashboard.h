const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
  <head>
    <title>SMART FARM DASHBOARD</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      :root {
        --primary-color: #2E7D32; 
        --secondary-color: #E8F5E9; 
        --text-dark: #333;
        --card-bg: #ffffff;
      }
      html {font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f7f6; color: var(--text-dark);}
      body {margin: 0; padding-bottom: 30px;}
      h3, h4, p {margin: 10px 0;}
      
      /* Header */
      .topnav {background-color: var(--primary-color); color: white; text-align: center; padding: 15px 0; box-shadow: 0 4px 6px rgba(0,0,0,0.1);}
      .topnav h3 {margin: 0; font-size: 1.5rem; letter-spacing: 1px;}
      
      /* Main Layout */
      .content {padding: 20px; max-width: 1200px; margin: 0 auto;}
      .cards {display: grid; grid-gap: 20px; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));}
      
      /* Node Cards */
      .card {background-color: var(--card-bg); border-radius: 12px; box-shadow: 0 5px 15px rgba(0,0,0,0.05); overflow: hidden; transition: transform 0.3s;}
      .card:hover {transform: translateY(-5px);}
      .card-header {background-color: var(--primary-color); color: white; padding: 15px; text-align: center;}
      .card-header h3 {margin: 0; font-size: 1.3rem;}
      .card-body {padding: 20px;}
      
      /* Sensor Grid */
      .sensor-grid {display: grid; grid-template-columns: 50% 50%; grid-gap: 15px; margin-bottom: 20px;}
      .sensor-item {background: var(--secondary-color); padding: 15px; border-radius: 8px; text-align: center; border: 1px solid #c8e6c9;}
      .sensor-item h4 {font-size: 0.85rem; color: #555; text-transform: uppercase;}
      .sensor-item .reading {font-size: 1.5rem; font-weight: bold;}
      
      .tempColor {color: #e53935;}
      .humColor {color: #1e88e5;}
      .lightColor {color: #fbc02d;}
      .soilColor {color: #43a047;}
      
      /* Chart Area */
      .chart-container {position: relative; height: 220px; width: 100%; margin-bottom: 20px;}

      /* Controls Area */
      .control-section {background: #f9f9f9; padding: 15px; border-radius: 8px; margin-bottom: 20px;}
      .control-row {display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;}
      .control-label {font-weight: bold; color: var(--primary-color);}
      
      /* Modern Toggle Switch */
      .switch {position: relative; display: inline-block; width: 54px; height: 28px;}
      .switch input {opacity: 0; width: 0; height: 0;}
      .sliderTS {position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px;}
      .sliderTS:before {position: absolute; content: ""; height: 20px; width: 20px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%;}
      input:checked + .sliderTS {background-color: var(--primary-color);}
      input:checked + .sliderTS:before {transform: translateX(26px);}
      input:disabled + .sliderTS {opacity: 0.5; cursor: not-allowed;}
      
      /* Status Info */
      .status-info {font-size: 0.85rem; color: #666; background: #fff; border-top: 1px solid #eee; padding-top: 15px;}
      .status-info p {margin: 5px 0; display: flex; justify-content: space-between;}
      
      #Show_Info {text-align: center; font-style: italic; color: var(--primary-color); margin-top: 20px; height: 20px;}
    </style>
  </head>
  
  <body>
    <div class="topnav">
      <h3>SMART FARM MONITORING SYSTEM</h3>
    </div>
    
    <div class="content">
      <div class="cards">
        
        <div class="card">
          <div class="card-header">
            <h3>NODE 1</h3>
          </div>
          <div class="card-body">
            
            <div class="sensor-grid">
              <div class="sensor-item">
                <h4>TEMPERATURE</h4>
                <div class="reading tempColor"><span id="temp_Slave_1">--</span>&deg;C</div>
              </div>
              <div class="sensor-item">
                <h4>HUMIDITY</h4>
                <div class="reading humColor"><span id="humd_Slave_1">--</span>%</div>
              </div>
              <div class="sensor-item">
                <h4>LIGHT INTENSITY</h4>
                <div class="reading lightColor"><span id="light_Slave_1">--</span> lux</div>
              </div>
              <div class="sensor-item">
                <h4>SOIL MOISTURE</h4>
                <div class="reading soilColor"><span id="soil_Slave_1">--</span>%</div>
              </div>
            </div>

            <div class="chart-container">
              <canvas id="chart_S1"></canvas>
            </div>
            
            <div class="control-section">
              <div class="control-row">
                <span class="control-label">Water Pump (LED 1):</span>
                <label class="switch">
                  <input type="checkbox" id="togLED_1_Slave_1" onclick="send_LED_State('togLED_1_Slave_1','S1','1')">
                  <div class="sliderTS"></div>
                </label>
              </div>
              <div class="control-row">
                <span class="control-label">Cooling Fan (LED 2):</span>
                <label class="switch">
                  <input type="checkbox" id="togLED_2_Slave_1" onclick="send_LED_State('togLED_2_Slave_1','S1','2')">
                  <div class="sliderTS"></div>
                </label>
              </div>
            </div>
            
            <div class="status-info">
              <p><b>Last Updated:</b> <span id="LTRD_Slave_1">--:--:--</span></p>
              <p><b>Sensor Status:</b> <span id="status_read_DHT11_Slave_1">--</span></p>
              <p>Pump Status: <span id="LED_1_State_Slave_1" style="font-weight:bold;">--</span></p>
              <p>Fan Status: <span id="LED_2_State_Slave_1" style="font-weight:bold;">--</span></p>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-header">
            <h3>NODE 2</h3>
          </div>
          <div class="card-body">
            
            <div class="sensor-grid">
              <div class="sensor-item">
                <h4>TEMPERATURE</h4>
                <div class="reading tempColor"><span id="temp_Slave_2">--</span>&deg;C</div>
              </div>
              <div class="sensor-item">
                <h4>HUMIDITY</h4>
                <div class="reading humColor"><span id="humd_Slave_2">--</span>%</div>
              </div>
              <div class="sensor-item">
                <h4>LIGHT INTENSITY</h4>
                <div class="reading lightColor"><span id="light_Slave_2">--</span> lux</div>
              </div>
              <div class="sensor-item">
                <h4>SOIL MOISTURE</h4>
                <div class="reading soilColor"><span id="soil_Slave_2">--</span>%</div>
              </div>
            </div>

            <div class="chart-container">
              <canvas id="chart_S2"></canvas>
            </div>
            
            <div class="control-section">
              <div class="control-row">
                <span class="control-label">Water Pump (LED 1):</span>
                <label class="switch">
                  <input type="checkbox" id="togLED_1_Slave_2" onclick="send_LED_State('togLED_1_Slave_2','S2','1')">
                  <div class="sliderTS"></div>
                </label>
              </div>
              <div class="control-row">
                <span class="control-label">Cooling Fan (LED 2):</span>
                <label class="switch">
                  <input type="checkbox" id="togLED_2_Slave_2" onclick="send_LED_State('togLED_2_Slave_2','S2','2')">
                  <div class="sliderTS"></div>
                </label>
              </div>
            </div>
            
            <div class="status-info">
              <p><b>Last Updated:</b> <span id="LTRD_Slave_2">--:--:--</span></p>
              <p><b>Sensor Status:</b> <span id="status_read_DHT11_Slave_2">--</span></p>
              <p>Pump Status: <span id="LED_1_State_Slave_2" style="font-weight:bold;">--</span></p>
              <p>Fan Status: <span id="LED_2_State_Slave_2" style="font-weight:bold;">--</span></p>
            </div>
          </div>
        </div>
        
      </div>
    </div>
    
    <p id="Show_Info">Waiting for connection...</p>
    
    <script>
      // Initialize Charts
      const maxDataPoints = 15; // Max points on the X axis
      
      const chartConfig = {
        type: 'line',
        data: {
          labels: [],
          datasets: [
            { label: 'Temp (°C)', borderColor: '#e53935', backgroundColor: '#e53935', data: [], tension: 0.4, fill: false, pointRadius: 2 },
            { label: 'Humidity (%)', borderColor: '#1e88e5', backgroundColor: '#1e88e5', data: [], tension: 0.4, fill: false, pointRadius: 2 },
            { label: 'Light (lux)', borderColor: '#fbc02d', backgroundColor: '#fbc02d', data: [], tension: 0.4, fill: false, pointRadius: 2 },
            { label: 'Soil (%)', borderColor: '#43a047', backgroundColor: '#43a047', data: [], tension: 0.4, fill: false, pointRadius: 2 }
          ]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          animation: false,
          scales: {
            x: { display: true },
            y: { beginAtZero: true, suggestedMax: 100 }
          },
          plugins: {
            legend: { 
                display: true, 
                position: 'top',
                labels: { boxWidth: 10, font: {size: 10} }
            }
          }
        }
      };

      const ctx1 = document.getElementById('chart_S1').getContext('2d');
      const chartS1 = new Chart(ctx1, JSON.parse(JSON.stringify(chartConfig)));

      const ctx2 = document.getElementById('chart_S2').getContext('2d');
      const chartS2 = new Chart(ctx2, JSON.parse(JSON.stringify(chartConfig)));

      function updateChart(chart, timeLabel, tempVal, humVal, lightVal, soilVal) {
        chart.data.labels.push(timeLabel);
        chart.data.datasets[0].data.push(tempVal);
        chart.data.datasets[1].data.push(humVal);
        chart.data.datasets[2].data.push(lightVal);
        chart.data.datasets[3].data.push(soilVal);
        
        if (chart.data.labels.length > maxDataPoints) {
          chart.data.labels.shift();
          chart.data.datasets[0].data.shift();
          chart.data.datasets[1].data.shift();
          chart.data.datasets[2].data.shift();
          chart.data.datasets[3].data.shift();
        }
        chart.update();
      }

      // Original System Logic
      var Timer = setInterval(myTimer, 100);
      var TimerShowInfo = setInterval(myTimerShowInfo, 100);
      var count_to_Ctrl_TglBtn = 0;
      var count_to_Show_Info = 0;
      var count_to_Show_Info_no_Data_is_coming = 0;
      let change_TglBtn_Disable = false;
      let start_Show_Info_For_First_Time = true;

      if (!!window.EventSource) {
        var source = new EventSource('/events');
        
        source.addEventListener('open', function(e) { console.log("Events Connected"); }, false);
        source.addEventListener('error', function(e) {
          if (e.target.readyState != EventSource.OPEN) { console.log("Events Disconnected"); }
        }, false);
        
        source.addEventListener('allDataJSON', function(e) {
          var today = new Date();
          var time = leading_zeros(today.getHours()) + ":" + leading_zeros(today.getMinutes()) + ":" + leading_zeros(today.getSeconds());

          count_to_Show_Info_no_Data_is_coming = 0;
          if (document.getElementById("Show_Info").innerHTML == "No data received for over 10 seconds.") {
            document.getElementById("Show_Info").innerHTML = "";
          }
          
          var obj = JSON.parse(e.data);
          
          if (obj.ID_Slave == "S1") {
            document.getElementById("status_read_DHT11_Slave_1").innerHTML = obj.StatusReadDHT11;
            document.getElementById("status_read_DHT11_Slave_1").style.color = (obj.StatusReadDHT11 == "FAILED") ? "red" : "#2E7D32";
            
            document.getElementById("humd_Slave_1").innerHTML = obj.Humd;
            document.getElementById("temp_Slave_1").innerHTML = obj.Temp.toFixed(1);
            document.getElementById("light_Slave_1").innerHTML = obj.Light;
            document.getElementById("soil_Slave_1").innerHTML = obj.SoilMoisture;

            document.getElementById("LED_1_State_Slave_1").innerHTML = obj.LED1 ? "ON" : "OFF";
            document.getElementById("LED_1_State_Slave_1").style.color = obj.LED1 ? "#2E7D32" : "#e53935";
            document.getElementById("togLED_1_Slave_1").checked = obj.LED1;

            document.getElementById("LED_2_State_Slave_1").innerHTML = obj.LED2 ? "ON" : "OFF";
            document.getElementById("LED_2_State_Slave_1").style.color = obj.LED2 ? "#2E7D32" : "#e53935";
            document.getElementById("togLED_2_Slave_1").checked = obj.LED2;
            
            document.getElementById("LTRD_Slave_1").innerHTML = time;
            
            // Update Chart
            updateChart(chartS1, time, obj.Temp, obj.Humd, obj.Light, obj.SoilMoisture);
          }
          
          if (obj.ID_Slave == "S2") {
            document.getElementById("status_read_DHT11_Slave_2").innerHTML = obj.StatusReadDHT11;
            document.getElementById("status_read_DHT11_Slave_2").style.color = (obj.StatusReadDHT11 == "FAILED") ? "red" : "#2E7D32";
            
            document.getElementById("humd_Slave_2").innerHTML = obj.Humd;
            document.getElementById("temp_Slave_2").innerHTML = obj.Temp.toFixed(1);
            document.getElementById("light_Slave_2").innerHTML = obj.Light;
            document.getElementById("soil_Slave_2").innerHTML = obj.SoilMoisture;
            
            document.getElementById("LED_1_State_Slave_2").innerHTML = obj.LED1 ? "ON" : "OFF";
            document.getElementById("LED_1_State_Slave_2").style.color = obj.LED1 ? "#2E7D32" : "#e53935";
            document.getElementById("togLED_1_Slave_2").checked = obj.LED1;

            document.getElementById("LED_2_State_Slave_2").innerHTML = obj.LED2 ? "ON" : "OFF";
            document.getElementById("LED_2_State_Slave_2").style.color = obj.LED2 ? "#2E7D32" : "#e53935";
            document.getElementById("togLED_2_Slave_2").checked = obj.LED2;
            
            document.getElementById("LTRD_Slave_2").innerHTML = time;

            // Update Chart
            updateChart(chartS2, time, obj.Temp, obj.Humd, obj.Light, obj.SoilMoisture);
          }
        }, false);
      }

      function requestLEDStatus() {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "get_LED_Status", true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState == 4 && xhr.status == 200) {
            var obj = JSON.parse(xhr.responseText);
            document.getElementById("togLED_1_Slave_1").checked = obj.LED1;
            document.getElementById("togLED_2_Slave_1").checked = obj.LED2;
            document.getElementById("togLED_1_Slave_2").checked = obj.LED1;
            document.getElementById("togLED_2_Slave_2").checked = obj.LED2;
          }
        };
        xhr.send();
      }
      window.onload = requestLEDStatus;

      function myTimer() {
        if (change_TglBtn_Disable == true) {
          count_to_Ctrl_TglBtn += 100;
          if (count_to_Ctrl_TglBtn > 1000) {
            count_to_Ctrl_TglBtn = 0;
            change_TglBtn_Disable = false;
            document.getElementById("togLED_1_Slave_1").disabled = false;
            document.getElementById("togLED_2_Slave_1").disabled = false;
            document.getElementById("togLED_1_Slave_2").disabled = false;
            document.getElementById("togLED_2_Slave_2").disabled = false; 
          }
        }
      }

      function myTimerShowInfo() {
        count_to_Show_Info_no_Data_is_coming += 100;
        if (count_to_Show_Info_no_Data_is_coming > 10000) {
          count_to_Show_Info_no_Data_is_coming = 0;
          document.getElementById("Show_Info").innerHTML = "No data received for over 10 seconds.";
        }
        
        count_to_Show_Info += 100;
        if (count_to_Show_Info > 1500) {
          count_to_Show_Info = 0;
          if (start_Show_Info_For_First_Time == true) {
            start_Show_Info_For_First_Time = false;
            document.getElementById("Show_Info").innerHTML = "";
            return;
          }
          if (document.getElementById("Show_Info").innerHTML.includes("Sending control command")) {
            document.getElementById("Show_Info").innerHTML = "Please wait for Node response...";
            return;
          }
          if (document.getElementById("Show_Info").innerHTML == "Please wait for Node response...") {
            document.getElementById("Show_Info").innerHTML = "";
            return;
          }
        }
      }

      function leading_zeros(x) { 
        return (x < 10 ? '0' : '') + x;
      }

      function send_LED_State(id,slave,led_num) {
        count_to_Show_Info = 0;
        change_TglBtn_Disable = true;
        document.getElementById("Show_Info").innerHTML = "Sending control command to " + (slave == "S1" ? "Node 1" : "Node 2") + "...";
        
        document.getElementById("togLED_1_Slave_1").disabled = true;
        document.getElementById("togLED_2_Slave_1").disabled = true;
        document.getElementById("togLED_1_Slave_2").disabled = true;
        document.getElementById("togLED_2_Slave_2").disabled = true;  
        
        var tgLEDFlash = document.getElementById(id);
        var tgState = tgLEDFlash.checked ? "t" : "f";
        send_cmd(slave,led_num,tgState);
      }

      function send_cmd(slave,led_num,value) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "set_LED?Slave_Num="+slave+"&LED_Num="+led_num+"&LED_Val="+value, true);
        xhr.send();
      }
    </script>
  </body>
</html>
)=====";