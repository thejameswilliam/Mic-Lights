// Real mic (MAX4466 analog OUT) -> volume (p2p) -> LED brightness (all channels)
// PWM polarity confirmed: ACTIVE-LOW (duty 0 = ON, duty 1023 = OFF)

const int MIC_PIN = 34;  // MAX4466 OUT to GPIO34 (ADC1)

const int R_PIN = 27;
const int G_PIN = 32;
const int B_PIN = 33;

//Bass Beats
float avgLevel = 0;
bool beat = false;

// tuning knobs
float beatSensitivity = 1.8;   // higher = fewer beats
float beatDecay = 0.05;        // how fast average adapts
unsigned long lastBeatTime = 0;
int beatHoldMs = 120;          // keep beat active for this long

// PWM
const int PWM_FREQ = 5000;
const int PWM_BITS = 10;
const int PWM_MAX  = (1 << PWM_BITS) - 1;

// Your tuned values (from your working simulation)
float noiseFloor = 400;   // p2p below this = silence
float loudLevel  = 1800;  // p2p around this = very loud

float smoothP2P = 0;

float clamp01(float x) {
  if (x < 0) return 0;
  if (x > 1) return 1;
  return x;
}

// brightness 0..PWM_MAX (0=off, max=full bright)
// active-low duty: 0=on, max=off
int dutyFromBrightness(int brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > PWM_MAX) brightness = PWM_MAX;
  return PWM_MAX - brightness;
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(MIC_PIN, ADC_11db);

  ledcAttach(R_PIN, PWM_FREQ, PWM_BITS);
  ledcAttach(G_PIN, PWM_FREQ, PWM_BITS);
  ledcAttach(B_PIN, PWM_FREQ, PWM_BITS);

  // Start OFF
  ledcWrite(R_PIN, PWM_MAX);
  ledcWrite(G_PIN, PWM_MAX);
  ledcWrite(B_PIN, PWM_MAX);
}

void loop() {
  // Measure peak-to-peak over a short window
  int minV = 4095;
  int maxV = 0;

  for (int i = 0; i < 100; i++) {
    int v = analogRead(MIC_PIN);
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
    delayMicroseconds(150);
  }

  // Smooth so the LEDs don't flicker
  int p2p = maxV - minV;

  // Attack/Release smoothing (snappy)
  float attack  = 0.65f;
  float release = 0.10f;

  if (p2p > smoothP2P) {
    smoothP2P = smoothP2P * (1.0f - attack) + p2p * attack;
  } else {
    smoothP2P = smoothP2P * (1.0f - release) + p2p * release;
  }

  // --- Beat detection ---
  avgLevel = avgLevel * (1.0 - beatDecay) + smoothP2P * beatDecay;

  beat = false;

  if (smoothP2P > avgLevel * beatSensitivity) {
    if (millis() - lastBeatTime > beatHoldMs) {
      beat = true;
      lastBeatTime = millis();
    }
  }
  

  // Map to brightness (0..1)
  float x = (smoothP2P - noiseFloor) / (loudLevel - noiseFloor);
  x = clamp01(x);

  // Curve for nicer feel
  float brightness01 = sqrt(x);

  int brightness = (int)(brightness01 * PWM_MAX);
  int duty = dutyFromBrightness(brightness);


  // --- Simple rainbow color loop ---
  int t = (millis() / 20) % 256;  // speed of color change (bigger = slower)

  int r, g, b;

  if (t < 85) {
    r = 255 - t * 3;
    g = t * 3;
    b = 0;
  }
  else if (t < 170) {
    int tt = t - 85;
    r = 0;
    g = 255 - tt * 3;
    b = tt * 3;
  }
  else {
    int tt = t - 170;
    r = tt * 3;
    g = 0;
    b = 255 - tt * 3;
  }


  if (beat) {
    brightness01 = min(1.0, brightness01 * 1.5);
  }

  // Apply sound brightness
  int rPWM = map((int)(r * brightness01), 0, 255, 0, PWM_MAX);
  int gPWM = map((int)(g * brightness01), 0, 255, 0, PWM_MAX);
  int bPWM = map((int)(b * brightness01), 0, 255, 0, PWM_MAX);

  

  // Active-low MOSFET outputs
  ledcWrite(R_PIN, PWM_MAX - rPWM);
  ledcWrite(G_PIN, PWM_MAX - gPWM);
  ledcWrite(B_PIN, PWM_MAX - bPWM);

  // // Optional: Serial Plotter tuning (comment out if you want)
  Serial.println(p2p);
  Serial.print(" ");
  Serial.print((int)smoothP2P);
  Serial.print(" ");
  Serial.print(noiseFloor);
  Serial.print(" ");
  Serial.println(loudLevel);

  delay(10);
}