/* 
 * PINS 
 */
const byte PIN_MOTEUR_L1 = 3;
const byte PIN_MOTEUR_L2 = 5;
const byte PIN_MOTEUR_R1 = 9;
const byte PIN_MOTEUR_R2 = 10;

const byte PIN_BALAIS = 2;
const byte PIN_BALAIS2 = 11;

const byte PIN_TRIGGER = 7;
const byte PIN_ECHO_LEFT = 8;
const byte PIN_ECHO_FRONT = 6;
const byte PIN_ECHO_RIGHT = 4;

/*
 * SYSTEM
 */
char mode = 'N';
int cpt = 0;

/*
 * ULTRASON
 */ 
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s
const float SOUND_SPEED = 340.0 / 1000; // mm/us
const int LEFT = 0;
const int FRONT = 1;
const int RIGHT = 2;
const int BACK = -1;
float dists[3]; // LEFT, FRONT, RIGHT in mm
const int TS_DETECT[3] = {120, 170, 120};
const int TS_CONTACT[3] = {90, 90, 90};


void _trigger(){digitalWrite(PIN_TRIGGER, HIGH); delayMicroseconds(10); digitalWrite(PIN_TRIGGER, LOW);}


float _udist(int pin){
  long t;
  _trigger();
  t = pulseIn(pin, HIGH, MEASURE_TIMEOUT);
  delay(40 - (t / 1000 ));
  if (t == 0)
    return 8000;
  else
    return t / 2.0 * SOUND_SPEED;
  
}

void udists() {
  dists[0] = (_udist(PIN_ECHO_LEFT) + _udist(PIN_ECHO_LEFT)) / 2.0 ;
  dists[1] = (_udist(PIN_ECHO_FRONT) + _udist(PIN_ECHO_FRONT)) / 2.0 ;
  dists[2] = (_udist(PIN_ECHO_RIGHT) + _udist(PIN_ECHO_RIGHT)) / 2.0 ;
  Serial.println(dists[0]);
  Serial.println(dists[1]);
  Serial.println(dists[2]);
  Serial.println("######");
}


/* 
 * MOTEURS 
 */
const byte SPEED_NORM = 180;
const byte SPEED_PLUS = 180;
const byte SPEED_MAX = 255;

void ml_on(byte sped=SPEED_NORM+40){digitalWrite(PIN_MOTEUR_L1, LOW); analogWrite(PIN_MOTEUR_L2,sped);}
void ml_rev(byte sped=SPEED_NORM+40){analogWrite(PIN_MOTEUR_L1,sped); digitalWrite(PIN_MOTEUR_L2, LOW);}
void ml_off(){digitalWrite(PIN_MOTEUR_L1, LOW); digitalWrite(PIN_MOTEUR_L2, LOW);}
void mr_on(byte sped=SPEED_NORM){digitalWrite(PIN_MOTEUR_R1, LOW); analogWrite(PIN_MOTEUR_R2,sped);}
void mr_rev(byte sped=SPEED_NORM){analogWrite(PIN_MOTEUR_R1,sped); digitalWrite(PIN_MOTEUR_R2, LOW);}
void mr_off(){digitalWrite(PIN_MOTEUR_R1, LOW); digitalWrite(PIN_MOTEUR_R2, LOW);}

/* 
 * BALAIS 
 */
void b_on(){digitalWrite(PIN_BALAIS, HIGH);}
void b_off(){digitalWrite(PIN_BALAIS, LOW);}


/*
 * MAIN
 */
void tourner(int dir){
  if (dir == FRONT){
    ml_on();
    mr_on();
  } else if (dir == RIGHT){
    ml_on();
    mr_off();
  } else if (dir == LEFT) {
    ml_off();
    mr_on();
  } else {
    ml_rev();
    mr_rev();
    delay(1000);
    ml_rev();
    mr_on();
    delay(1000);
  }
}

void recule(int dir){
  if (dir == FRONT){
    mr_rev();
    ml_rev();
  } else if (dir == RIGHT){
    mr_off();
    ml_rev();
  } else if (dir == LEFT) {
    mr_rev();
    ml_off();
  } 
}

void stop(){mr_off(); ml_off();}

boolean detecte_contourne(int dir){
  if (dists[dir] < TS_CONTACT[dir]){
    recule(dir);
    delay(1000);
  }
  if (dists[dir] < TS_DETECT[dir]){
    if (dists[(dir - 1) % 3] > TS_DETECT[(dir - 1) % 3]){
      tourner((dir - 1) % 3);
    } else if (dists[(dir + 1) % 3] > TS_DETECT[(dir + 1) % 3]) {
      tourner((dir + 1) % 3);
    } else {
      tourner(BACK);
    }
    delay(1000);
    tourner(FRONT);
    return true;
  }
  return false;
}

void mode_n(){mode = 'N'; cpt = 0;}

void mode_c(){mode = 'C'; cpt = 0;}

/*
 * SETUP
 */
void setup() {
  pinMode(13,OUTPUT);
  //digitalWrite(13, HIGH);
  Serial.begin(115200);
  
  // ULTRASON
  pinMode(PIN_TRIGGER, OUTPUT);
  digitalWrite(PIN_TRIGGER, LOW);
  pinMode(PIN_ECHO_LEFT, INPUT);
  pinMode(PIN_ECHO_FRONT, INPUT);
  pinMode(PIN_ECHO_RIGHT, INPUT);

  // MOTEUR
  pinMode(PIN_MOTEUR_R1, OUTPUT); 
  pinMode(PIN_MOTEUR_R2, OUTPUT);
  pinMode(PIN_MOTEUR_L1, OUTPUT); 
  pinMode(PIN_MOTEUR_L2, OUTPUT);

  // BALAIS
  pinMode(PIN_BALAIS, OUTPUT);
  pinMode(PIN_BALAIS2, OUTPUT);
  digitalWrite(PIN_BALAIS, LOW);
  digitalWrite(PIN_BALAIS2, LOW);
  Serial.println("coucou");
  delay(3000);
  Serial.println("Coucou");
  b_on();
  //delay(3000);
  //Serial.println("Coucou");
  //delay(300000);
  
}


/*
 * MAIN LOOP
 */
void loop() {
  
  udists();
  //Serial.println(dists[1]);
  // Détection obstacle et contournement
  boolean detect = false;
  for (int i = 0; i < 3; i++){
    detect |= detecte_contourne(i);
  }

  if (detect == false){
    ml_on();
    mr_on();
  }
  // Si obstacle, on repasse en mode normal
  if (detect == true) {
    mode_n();
  }
  
  // Si on est pas en train de contourner un obstacle
  if (detect == false){

    // Si mode normal et au milieu de la pièce, on passe en mode cycle
    if (mode == 'N') {
      if (dists[0] > 1000 && dists[1] > 1000 && dists[2] > 1000){
        mode_c();
      } else {
        tourner(FRONT);
      }
    }

    // Si mode C, on continue
    if (mode == 'C'){
      mr_on(SPEED_PLUS);
      // start left motor 3s after
      if (cpt < 30) {
        ml_off();
      } else {
        ml_on();
      }
    }
  }

  // EMERGENCY
  if (cpt > 200){
    b_off();
    ml_rev(SPEED_MAX);
    mr_rev(SPEED_MAX);
    delay(2000);
    tourner(BACK);
    b_on();
    cpt = 0;
  }
  cpt++;
}
