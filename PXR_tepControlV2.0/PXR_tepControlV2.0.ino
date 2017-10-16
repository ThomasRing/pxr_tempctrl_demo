#include <DueTimer.h>

unsigned char sensorValue = 0;
unsigned char crcHigh;
unsigned char crcLow;
unsigned char PXR_cmd[8] = {0x04,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
float value,total_value;
unsigned int n;

float SV_upper=30;
float SV_lower=28;
float SV_data,PV_data;

unsigned int changeTime_low=200;
unsigned int changeTime_up=200;
unsigned int completeTime;
unsigned char initData;
unsigned char changeFlag,waitFlag,initFlag,cirFlag;

unsigned int readRegister(unsigned char id,unsigned char fun,unsigned int startAdress,unsigned char registerCounter);
unsigned char writeRegister(unsigned char id,unsigned char fun,unsigned int starAdress,unsigned int registerState);
void tem_init(float tem);
void readPXR_data(void);
void readCmd(void);
void handler2(void);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  while (!Serial1)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  tem_init(30);

  Timer4.start();
  Timer4.attachInterrupt(handler2).setFrequency(1).start();
}

void loop() {
  // put your main code here, to run repeatedly:
   static unsigned int m;
    int data=analogRead(sensorValue);
    float vol=data*3.3*0.977;
    float pressure=(vol/100-4)/16*500;
    total_value+=pressure;
    if(++n>=300) {
      n=0;
      pressure=total_value/300;
      value=pressure;
      total_value=0;
    }
    
   if(++m>60000){
    m=0;
    readPXR_data();
   }

   if(initFlag){
    initFlag=0;
    tem_init(initData);
   }
}
void readCmd(){
  unsigned char cmd,data;
  cmd=Serial.read();
  data=Serial.read();
  switch(cmd)
  {
    case 0x01:             //Set SV
       initFlag=1;
       initData=data;
       break;
    case 0x02:             //Set SV_upper
       SV_upper=data;
       break;
    case 0x03:              //Set SV_lower
       SV_lower=data;
       break;
    case 0x04:       //Set the time on upper temperature
       changeTime_up=data;
       break;
    case 0x05:            //Set the time on lower temperature
       changeTime_low=data;
       break;    
    case 0x06:
       cirFlag=0;
      // Serial.print(0);
       break;
    case 0x07:
       cirFlag=1;
      //  Serial.print(1);
       break;
     default:break;    
  }
}
void readPXR_data(){
    unsigned int Temp = 0;
    Temp =readRegister(0x02,0x04,0x0000,0x01);
    PV_data=(float)(Temp)/100*5;
   // Serial.println(PV_data);
    Temp =readRegister(0x02,0x04,0x0001,0x01);
    SV_data=(float)(Temp)/100*5;
    //Serial.println(SV_data);
    if(cirFlag){
    if(SV_data<=PV_data&&SV_data==SV_upper&&waitFlag==0){
       changeFlag=1;
       waitFlag=1;
    }
    if(SV_data>=PV_data&&SV_data==SV_lower&&waitFlag==0){
       changeFlag=2;
       waitFlag=1;
    }
    if(changeFlag==3){
        while(!(writeRegister(0x02,0x06,0x0002,20*SV_lower)));
        changeFlag=0;
        waitFlag=0;
    }
    if(changeFlag==4){
      while(!(writeRegister(0x02,0x06,0x0002,20*SV_upper)));
        changeFlag=0;
        waitFlag=0;
    }
    }
}


void handler2(){
  Serial.println(value);
 /* Serial.print("SV_lower:");
  Serial.println(SV_lower);
  Serial.print("SV_upper:");
  Serial.println(SV_upper);
  Serial.print("changeTime_low:");
  Serial.println(changeTime_low);
  Serial.print("changeTime_up:");
  Serial.println(changeTime_up);*/
  if(Serial.available()>0){
      readCmd();
    }
    if(cirFlag){
  if(changeFlag==1&&waitFlag==1){
    if(++completeTime>=changeTime_up*60){
        completeTime=0;
        changeFlag=3;
    }
  }
  if(changeFlag==2&&waitFlag==1){
    if(++completeTime>=changeTime_low*60){
        completeTime=0;
        changeFlag=4;
    }
  }
     }
}

void CRC16(unsigned char *ptr,unsigned int len){
  unsigned int wcrc = 0xFFFF;
  unsigned char temp;
  int i = 0,j = 0;
  for(i=0;i<len;i++){
    temp = *ptr&0xFF;
    ptr++;
    wcrc ^=temp;
    for(j=0;j<8;j++){
      if(wcrc&0x0001){
        wcrc>>=1;
        wcrc^=0xA001;
      }
      else{
        wcrc>>=1;
      }
    }
  }
  temp = wcrc;
  crcHigh = wcrc;
  crcLow = wcrc>>8;
}

unsigned char writeRegister(unsigned char id,unsigned char fun,unsigned int starAdress,unsigned int registerState)
{
  unsigned char ii;
  unsigned char PXR_GET_data[8];
  PXR_cmd[0] = id;//设置需要读取的从机设备的地址
  PXR_cmd[1] = fun;//设置功能码
  PXR_cmd[2] =(unsigned char)(starAdress>>8);//设置起始地址的高位为0；
  PXR_cmd[3] =(unsigned char)(starAdress&0x00ff);//设置起始地址的低位；
  PXR_cmd[4] =(unsigned char)(registerState>>8);//设置结束地址的高位为0
  PXR_cmd[5] =(unsigned char)(registerState&0x00ff);//设置起结束地址的低位；
  CRC16(PXR_cmd,6);
  PXR_cmd[6] = crcHigh;//设置结束地址的高位为0
  PXR_cmd[7] = crcLow;//设置起结束地址的低位；
  for(ii=0;ii<8;ii++){
    Serial1.write(PXR_cmd[ii]);
  }
   delay(100);

   ii=0;
   if(Serial1.available()){
   while(Serial1.available()){
    PXR_GET_data[ii]=Serial1.read();
    ii++;
    if(ii>=8){
      Serial1.flush();
      break;
    }
  }
  CRC16(PXR_GET_data,6);
  if((PXR_GET_data[6]==crcHigh)&&(PXR_GET_data[7]==crcLow)){
    return 1;
  }
  }
  else{
    return 0;
  }
}

unsigned int readRegister(unsigned char id,unsigned char fun,unsigned int startAdress,unsigned char registerCounter)
{
  unsigned char ii=0;
  unsigned char dataLength = (registerCounter<<1)+5;
  unsigned char PXR_GET_data[dataLength];
  PXR_cmd[0] = id;//设置需要读取的从机设备的地址
  PXR_cmd[1] = fun;//设置功能码
  PXR_cmd[2] = startAdress>>8;//设置起始地址的高位为0；
  PXR_cmd[3] = startAdress&0x00ff;//设置起始地址的低位；
  PXR_cmd[4] = 0x00;//设置结束地址的高位为0
  PXR_cmd[5] = registerCounter;//设置起结束地址的低位；
  CRC16(PXR_cmd,6);
  PXR_cmd[6] = crcHigh;//设置结束地址的高位为0
  PXR_cmd[7] = crcLow;//设置起结束地址的低位；
  for(ii=0;ii<8;ii++){
    Serial1.write(PXR_cmd[ii]);
  }
  delay(100);
  ii=0;
  if(Serial1.available()){
  while(Serial1.available()){
    PXR_GET_data[ii]=Serial1.read();
    ii++;
    if(ii>=dataLength){
      Serial1.flush();
      break;
    }
  }
  CRC16(PXR_GET_data,dataLength-2);
  if((PXR_GET_data[dataLength-2]==crcHigh)&&(PXR_GET_data[dataLength-1]==crcLow)){
    return ((PXR_GET_data[3]<<8)+PXR_GET_data[4]);
  }
  }
  else{
    return 0;
  }
} 

void tem_init(float tem){
  writeRegister(0x02,0x06,0x0002,tem*20);
}
