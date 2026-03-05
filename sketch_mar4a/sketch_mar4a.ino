// Real mic (MAX4466 analog OUT) -> volume (p2p) -> LED brightness (all channels)
// PWM polarity confirmed: ACTIVE-LOW (duty 0 = ON, duty 1023 = OFF)

const int MIC_PIN = 34;  // MAX4466 OUT to GPIO34 (ADC1)

const int R_PIN = 27;
const int G_PIN = 32;
const int B_PIN = 33;

// PWM
const int PWM_FREQ = 5000;
const int PWM_BITS = 10;
const int PWM_MAX  = (1 << PWM_BITS) - 1;

// Your tuned values (from your working simulation)
float noiseFloor = 100;   // p2p below this = silence
float loudLevel  = 600;  // p2p around this = very loud

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

  for (int i = 0; i < 250; i++) {
    int v = analogRead(MIC_PIN);
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
    delayMicroseconds(150);
  }

  int p2p = maxV - minV;

  // Smooth so the LEDs don't flicker
  smoothP2P = smoothP2P * 0.85f + p2p * 0.15f;
  

  // Map to brightness (0..1)
  float x = (smoothP2P - noiseFloor) / (loudLevel - noiseFloor);
  x = clamp01(x);

  // Curve for nicer feel
  float brightness01 = x * x;

  int brightness = (int)(brightness01 * PWM_MAX);
  int duty = dutyFromBrightness(brightness);

  // All channels same brightness (white pulse)
  ledcWrite(R_PIN, duty);
  ledcWrite(G_PIN, duty);
  ledcWrite(B_PIN, duty);

  // Optional: Serial Plotter tuning (comment out if you want)
  Serial.println(duty);
  // Serial.print(" ");
  // Serial.print((int)smoothP2P);
  // Serial.print(" ");
  // Serial.print(noiseFloor);
  // Serial.print(" ");
  // Serial.println(loudLevel);

  delay(10);
}