const unsigned char Ramp[]={0,5,10,16,21,27,32,37,43,48,54,59,65,70,75,81,86,92,97,103,108,113,119,124,130,135,141,146,151,157,162,168,173,179,184,189,195,200,206,211,217,222,227,233,238,244,249,255};
const unsigned char InvRamp[]={255,249,244,238,233,227,222,217,211,206,200,195,189,184,179,173,168,162,157,151,146,141,135,130,124,119,113,108,103,97,92,86,81,75,70,65,59,54,48,43,37,32,27,21,16,10,5,0};
const unsigned char Triangle[]={0,10,21,32,43,54,65,75,86,97,108,119,130,141,151,162,173,184,195,206,217,227,238,249,255,249,238,227,217,206,195,184,173,162,151,141,130,119,108,97,86,75,65,54,43,32,21,10};
const unsigned char Rect[]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const unsigned char InvRect[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
const unsigned char Sine[]={0,1,4,10,17,27,38,51,66,81,97,114,131,148,165,181,196,209,222,232,241,247,252,254,254,252,247,241,232,222,209,196,181,165,148,131,114,97,81,66,51,38,27,17,10,4,1,0};
const unsigned char InvSine[]={254,253,250,244,237,227,216,203,188,173,157,140,123,106,89,73,58,45,32,22,13,7,2,0,0,2,7,13,22,32,45,58,73,89,106,123,140,157,173,188,203,216,227,237,244,250,253,254};

const unsigned char* waveform[]={
  Ramp,
  InvRamp,
  Triangle,
  Rect,
  InvRect,
  Sine,
  InvSine 
};

const char waveformName[][5]={
  { 'R','a','m','p',0 },
  { 'i','R','a','m',0 },
  { 'T','r','i','a',0 },
  { 'R','e','c','t',0 },
  { 'i','R','e','c',0 },
  { 'S','i','n','e',0 },
  { 'i','S','i','n',0 },
};
