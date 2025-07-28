#include <IRremote.hpp>

// Pinos
const int RECV_PIN = A5;
const int SEND_PIN = 3;       // Pino de envio IR (deve ter suporte PWM, como o D3 em Arduino Uno)
const int BUTTON_PIN = 2;

// Código armazenado
IRData receivedData;
uint16_t rawData[RAW_BUFFER_LENGTH];
int rawLength = 0;
int codeType = -1;
uint32_t codeValue = 0;
int codeBits = 0;
int toggle = 0;

int lastButtonState = LOW;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);

  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); // Inicia o receptor
  IrSender.begin(SEND_PIN);                        // Corrigido: inicia emissor IR

  lastButtonState = digitalRead(BUTTON_PIN);
  Serial.println("IR Receiver and Sender ready.");
}


void storeCode() {
  receivedData = IrReceiver.decodedIRData;
  codeType = receivedData.protocol;

  if (codeType == UNKNOWN) {
    Serial.println("Received unknown code, saving as raw");
    rawLength = IrReceiver.decodedIRData.rawDataPtr->rawlen;
    memcpy(rawData, IrReceiver.decodedIRData.rawDataPtr->rawbuf, rawLength * sizeof(uint16_t));

    Serial.print("Raw data (length ");
    Serial.print(rawLength);
    Serial.println("):");

    for (int i = 1; i < rawLength; i++) {
      Serial.print(rawData[i]);
      Serial.print(i % 2 == 0 ? "s " : "m ");
    }
    Serial.println();
  } else {
    Serial.print("Received ");
    Serial.print(getProtocolString(codeType));
    Serial.print(": ");
    Serial.println(receivedData.decodedRawData, HEX);

    codeValue = receivedData.decodedRawData;
    codeBits = receivedData.numberOfBits;
  }
}

void sendCode(bool repeat) {
  if (codeType == UNKNOWN) {
    IrSender.sendRaw(rawData + 1, rawLength - 1, 38);  // Envia ignorando o primeiro espaço
    Serial.println("Sent raw");
    return;
  }

  switch (codeType) {
    case NEC:
      if (repeat) {
        IrSender.sendNEC(REPEAT, codeBits);
        Serial.println("Sent NEC repeat");
      } else {
        IrSender.sendNEC(codeValue, codeBits);
        Serial.print("Sent NEC ");
        Serial.println(codeValue, HEX);
      }
      break;

    case SONY:
      IrSender.sendSony(codeValue, codeBits);
      Serial.print("Sent SONY ");
      Serial.println(codeValue, HEX);
      break;

    case RC5:
    case RC6:
      if (!repeat) {
        toggle = 1 - toggle;
      }
      codeValue = (codeValue & ~(1UL << (codeBits - 1))) | (toggle << (codeBits - 1));

      if (codeType == RC5) {
        IrSender.sendRC5(codeValue, codeBits);
        Serial.print("Sent RC5 ");
      } else {
        IrSender.sendRC6(codeValue, codeBits);
        Serial.print("Sent RC6 ");
      }
      Serial.println(codeValue, HEX);
      break;

    default:
      Serial.println("Unsupported protocol");
      break;
  }
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("Released");
    IrReceiver.start(); // Reativa o receptor
  }

  if (buttonState == HIGH) {
    Serial.println("Pressed, sending");
    sendCode(lastButtonState == buttonState);
    delay(50); // debounce simples
  } else if (IrReceiver.decode()) {
    storeCode();
    IrReceiver.resume();
  }

  lastButtonState = buttonState;
}


