#include <math.h>
#include <PDM.h>
#include <EloquentTinyML.h>      
#include "tf_lite_model.h"       // TF Lite model file


 #define RED 22     
 #define BLUE 24     
 #define GREEN 23

 
#define PDM_SOUND_GAIN     255   // sound gain of PDM mic
#define PDM_BUFFER_SIZE    256   // buffer size of PDM mic

#define SAMPLE_THRESHOLD   100   // RMS threshold to trigger sampling
#define FEATURE_SIZE       64    // sampling size of one voice instance
#define SAMPLE_DELAY       10    // delay time (ms) between sampling

#define NUMBER_OF_LABELS   2     // number of voice labels

#define PREDICT_THRESHOLD   0.6   // prediction probability threshold for labels
#define RAW_OUTPUT         true  // output prediction probability of each label
#define NUMBER_OF_INPUTS   FEATURE_SIZE
#define NUMBER_OF_OUTPUTS  NUMBER_OF_LABELS
#define TENSOR_ARENA_SIZE  4 * 1024


Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> tf_model;
float feature_data[FEATURE_SIZE];
volatile float rms;
bool voice_detected;

int coughs = 0;

void setup() {
 // intitialize the digital Pin as an output
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  digitalWrite(BLUE, HIGH);
  
  Serial.begin(115200);

  PDM.onReceive(onPDMdata);
  PDM.setBufferSize(PDM_BUFFER_SIZE);
  PDM.setGain(PDM_SOUND_GAIN);

  PDM.begin(1, 16000);   // start PDM mic and sampling at 16 KHz

  // wait 1 second to avoid initial PDM reading
  delay(1000);

  // start TF Lite model
  tf_model.begin((unsigned char*) model_data);

}

void loop() {

    Cough_Detection();

    if (coughs < 5)
    {
      digitalWrite(RED, HIGH); 
      digitalWrite(GREEN, LOW);
    }

    if (coughs >= 5 && coughs < 10)
    {
      digitalWrite(RED, LOW); 
      digitalWrite(GREEN, LOW);
    }

    if (coughs >= 10)
    {
      digitalWrite(RED, LOW); 
      digitalWrite(GREEN, HIGH);
    }
}




// callback function for PDM mic
void onPDMdata() {

  rms = -1;
  short sample_buffer[PDM_BUFFER_SIZE];
  int bytes_available = PDM.available();
  PDM.read(sample_buffer, bytes_available);

  // calculate RMS (root mean square) from sample_buffer
  unsigned int sum = 0;
  for (unsigned short i = 0; i < (bytes_available / 2); i++) sum += pow(sample_buffer[i], 2);
  rms = sqrt(float(sum) / (float(bytes_available) / 2.0));
}





void Cough_Detection()
{

  if (rms > SAMPLE_THRESHOLD)
  {
   
    digitalWrite(LED_BUILTIN, HIGH);
    for (int i = 0; i < FEATURE_SIZE; i++) {  // sampling
      while (rms < 0);
      feature_data[i] = rms;  
      delay(SAMPLE_DELAY);
    }
    digitalWrite(LED_BUILTIN, LOW); //When Sampling Is Done, Led Off
  
    // predict voice and put results (probability) for each label in the array
    float prediction[NUMBER_OF_LABELS];
    tf_model.predict(feature_data, prediction);
  
    voice_detected = false;
      if (prediction[0] >= PREDICT_THRESHOLD) {
        coughs++;
        Serial.print(coughs);
        Serial.print("\n");
        voice_detected = true;

      }
    // wait for 0.5 second after one sampling/prediction
    delay(500);
  }
  

  
}
