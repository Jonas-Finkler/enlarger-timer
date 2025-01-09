#include <Arduino.h>

String rootHtml = R"(
  <!DOCTYPE html>
  <html>
  <body>
    <div style='text-align:center;padding:20px'>
      <button onclick='toggleRelay()' style='font-size:24px;padding:20px;margin:10px'>Toggle Relay</button>
      <br>
      <button onclick='turnOn()' style='font-size:24px;padding:20px;margin:10px'>Turn On</button>
      <button onclick='turnOff()' style='font-size:24px;padding:20px;margin:10px'>Turn Off</button>
      <br><br>
      <div style='margin:20px'>
        <input type='number' id='timerInput' style='font-size:20px;padding:10px' placeholder='Enter milliseconds'>
        <button onclick='startTimer()' style='font-size:24px;padding:20px;margin:10px'>Start Timer</button>
      </div>
    </div>
    <script>
      function toggleRelay() {
        fetch('/toggle').then(response => console.log('toggled'));
      }
      function turnOn() {
        fetch('/on').then(response => console.log('turned on'));
      }
      function turnOff() {
        fetch('/off').then(response => console.log('turned off'));
      }
      function startTimer() {
        const ms = document.getElementById('timerInput').value;
        if (ms > 0) {
          fetch('/timer?miliseconds=' + ms)
            .then(response => console.log('timer started for ' + ms + 'ms'));
        } else {
          alert('Please enter a valid time in milliseconds');
        }
      }
    </script>
  </body>
  </html>
)";

