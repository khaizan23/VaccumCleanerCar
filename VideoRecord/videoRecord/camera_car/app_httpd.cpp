#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"
#include "time.h"

extern int leftmotor1;
extern int leftmotor2;
extern int rightmotor1;
extern int rightmotor2;
extern int vacuum;
extern String Camerafeed;
extern String direction ;
void Motor(int L1, int L2, int R1, int R2);

typedef struct {
  size_t size; //number of values used for filtering
  size_t index; //current value index
  size_t count; //value count
  int sum;
  int * values; //array to be filled with values
} ra_filter_t;

typedef struct {
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;
#define _HTTP_Method_H_
typedef enum {
  jHTTP_GET     = 0b00000001,
  jHTTP_POST    = 0b00000010,
  jHTTP_DELETE  = 0b00000100,
  jHTTP_PUT     = 0b00001000,
  jHTTP_PATCH   = 0b00010000,
  jHTTP_HEAD    = 0b00100000,
  jHTTP_OPTIONS = 0b01000000,
  jHTTP_ANY     = 0b01111111,
} HTTPMethod;

#include <WiFi.h>
#include <HTTPClient.h>
HTTPClient http;
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

time_t now;
struct tm timeinfo;

char the_page[4000];


static ra_filter_t * ra_filter_init(ra_filter_t * filter, size_t sample_size) {
  memset(filter, 0, sizeof(ra_filter_t));

  filter->values = (int *)malloc(sample_size * sizeof(int));
  if (!filter->values) {
    return NULL;
  }
  memset(filter->values, 0, sample_size * sizeof(int));

  filter->size = sample_size;
  return filter;
}

static int ra_filter_run(ra_filter_t * filter, int value) {
  if (!filter->values) {
    return value;
  }
  filter->sum -= filter->values[filter->index];
  filter->values[filter->index] = value;
  filter->sum += filter->values[filter->index];
  filter->index++;
  filter->index = filter->index % filter->size;
  if (filter->count < filter->size) {
    filter->count++;
  }
  return filter->sum / filter->count;
}

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len) {
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (!index) {
    j->len = 0;
  }
  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
    return 0;
  }
  j->len += len;
  return len;
}

static esp_err_t capture_handler(httpd_req_t *req) {

   long start = millis();
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
   char fname[100];
  int file_number = 0;
  int64_t fr_start = esp_timer_get_time();

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.printf("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  file_number++;

  sprintf(fname, "inline; filename=capture_%d.jpg", file_number);

  if (fb_next == NULL) {
    fb = get_good_jpeg(); // esp_camera_fb_get();
    framebuffer_len = fb->len;
    memcpy(framebuffer, fb->buf, framebuffer_len);
    esp_camera_fb_return(fb);
  } else {
    fb = fb_next;
    framebuffer_len = fb->len;
    memcpy(framebuffer, fb->buf, framebuffer_len);
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Content-Disposition", fname);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");  

  size_t fb_len = 0;
  if (fb->format == PIXFORMAT_JPEG) {
    fb_len = fb->len;
    res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  } else {
    jpg_chunking_t jchunk = {req, 0};
    res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
    httpd_resp_send_chunk(req, NULL, 0);
    fb_len = jchunk.len;
  }
  esp_camera_fb_return(fb);
  int64_t fr_end = esp_timer_get_time();
  Serial.printf("JPG: %uB %ums", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
   time_in_web1 += (millis() - start);
  return res;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  static int64_t last_frame = 0;
  if (!last_frame) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.printf("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if (fb->format != PIXFORMAT_JPEG) {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if (!jpeg_converted) {
          Serial.printf("JPEG compression failed");
          res = ESP_FAIL;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      break;
    }
    int64_t fr_end = esp_timer_get_time();

    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;
    frame_time /= 1000;
    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
    Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)"
                  , (uint32_t)(_jpg_buf_len),
                  (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                  avg_frame_time, 1000.0 / avg_frame_time
                 );
  }

  last_frame = 0;
  return res;
}

static esp_err_t cmd_handler(httpd_req_t *req) {
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};
  char value[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
          httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int val = atoi(value);
  sensor_t * s = esp_camera_sensor_get();
  int res = 0;

  if (!strcmp(variable, "framesize")) {
    if (s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
  }
  else if (!strcmp(variable, "quality")) res = s->set_quality(s, val);
  else if (!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
  else if (!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
  else if (!strcmp(variable, "saturation")) res = s->set_saturation(s, val);
  else if (!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
  else if (!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);
  else if (!strcmp(variable, "awb")) res = s->set_whitebal(s, val);
  else if (!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);
  else if (!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);
  else if (!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
  else if (!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
  else if (!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);
  else if (!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);
  else if (!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);
  else if (!strcmp(variable, "aec2")) res = s->set_aec2(s, val);
  else if (!strcmp(variable, "dcw")) res = s->set_dcw(s, val);
  else if (!strcmp(variable, "bpc")) res = s->set_bpc(s, val);
  else if (!strcmp(variable, "wpc")) res = s->set_wpc(s, val);
  else if (!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);
  else if (!strcmp(variable, "lenc")) res = s->set_lenc(s, val);
  else if (!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);
  else if (!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);
  else if (!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);
  else {
    res = -1;
  }

  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req) {
  static char json_response[1024];

  sensor_t * s = esp_camera_sensor_get();
  char * p = json_response;
  *p++ = '{';

  p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p += sprintf(p, "\"quality\":%u,", s->status.quality);
  p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
  p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
  p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
  p += sprintf(p, "\"awb\":%u,", s->status.awb);
  p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
  p += sprintf(p, "\"aec\":%u,", s->status.aec);
  p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
  p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
  p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
  p += sprintf(p, "\"agc\":%u,", s->status.agc);
  p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
  p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
  p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
  p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
  p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
  p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
  p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
  p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
  p += sprintf(p, "\"colorbar\":%u", s->status.colorbar);
  *p++ = '}';
  *p++ = 0;
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  String page = "";
  page += "<button onclick="hideIframe();stream.src=host+'/capture?'+Math.floor(Math.random()*1000000);">Get Still</button>";
  page += "<button onclick="hideIframe();stream.src=host+':81/stream';">Start Stream</button>";
  page += "<button onclick="hideIframe();stream.src='';">Stop Stream</button><br>";
  page += "<button onclick="hideIframe();fetch(host+'/control?restart');">Restart</button>";
  page += "<button onclick="hideIframe();getMessage(host+'/control?record');getRecordState();">Start Record</button>";
  page += "<button onclick="hideIframe();clearTimeout(recordTimer);getMessage(host+'/control?stop');">Stop Record</button><br>;"
  page += "<select id="command" onclick="execute();"><option value=""></option><option value="/list">List files</option><option value="/control?var=recordonce&val=1">Record once</option><option value="/control?var=recordonce&val=0">Record continuously</option><option value="/control?resetfilegroup">Reset file group</option></select>";
  page += "<span id="message" style="color:red"></span><br><img id="stream" src="" crossorigin="anonymous"><br>";
  page += "<iframe id="ifr" width="300" height="200" style="border: 0px solid black;display:none"></iframe>";
  page += "<script>var host=window.location.origin;var stream = document.getElementById("stream");var ifr = document.getElementById("ifr");var message = document.getElementById("message");var command = document.getElementById("command");var recordTimer;function execute() {message.innerHTML="";if (command.value!="") {hideIframe();if (command.value=="/list") {showIframe();ifr.src = host+command.value;}else{getMessage(host+command.value);}command.value="";}}function getMessage(url) {fetch(url).then(function(response) {return response.text();}).then(function(text) {if (text=="Do nothing") clearTimeout(recordTimer);message.innerHTML=text;});}function hideIframe() {ifr.style.display="none";}function showIframe() {ifr.style.display="block";}function getRecordState() {recordTimer = setTimeout(function() {getMessage(host+'/control?message');getRecordState();}, 2000);}</script>";
  page += "<TITLE>Vaccum cleaner Robot</TITLE>";
  page += "<H3 align='center'>vacuum cleaner Robot Using ESP32 cam</H3>";
  page += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0\">\n";
  page += "<script>var xhttp = new XMLHttpRequest();</script>";
  page += "<script>function getsend(arg) { xhttp.open('GET', arg +'?' + new Date().getTime(), true); xhttp.send() } </script><br>";

  page += "<p align=center><IMG SRC='http://" + Camerafeed + ":81/stream' style='width:370px;height:350px; transform:rotate(90deg);'></p><br/><br/>";

  page += "<p align=center> <button style=width:90px;height:30px; onmousedown=getsend('forward')  ontouchstart=getsend('forward') onmouseup=getsend('stop') ontouchend=getsend('stop') >Forward</button> </p>";
  page += "<p align=center>";
  page += "<button style=width:90px;height:30px onmousedown=getsend('leftturn')  ontouchstart=getsend('leftturn') onmouseup=getsend('stop') ontouchend=getsend('stop')>Left</button>&nbsp;";
  page += "<button style=width:90px;height:30px onmousedown=getsend('stop') onmouseup=getsend('stop')onmouseup=getsend('stop') ontouchend=getsend('stop')>Stop</button>&nbsp;";
  page += "<button style=width:90px;height:30px onmousedown=getsend('rightturn')  ontouchstart=getsend('rightturn') onmouseup=getsend('stop') ontouchend=getsend('stop')>Right</button>";
  page += "</p>";
  page += "<p align=center><button style=width:90px;height:30px onmousedown=getsend('reverse')  ontouchstart=getsend('reverse') onmouseup=getsend('stop') ontouchend=getsend('stop') >Reverse</button></p>";
  page += "</p>";

httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, (const char *)index_html, strlen(index_html));

  return httpd_resp_send(req, &page[0], strlen(&page[0]));
}

static esp_err_t forward_handler(httpd_req_t *req) {
  // Motor(LOW, HIGH, LOW, HIGH);
  digitalWrite(vacuum, HIGH);
  Serial.println("Forward");
  direction = "forward";
  Motor(LOW, HIGH, HIGH, LOW);
  //delay(4000);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "OK", 2);
}
static esp_err_t reverse_handler(httpd_req_t *req) {
  // Motor(HIGH, LOW, HIGH, LOW);
  digitalWrite(vacuum, HIGH);
  Serial.println("Reverse");
  direction = "reverse";
  Motor(HIGH, LOW, LOW, HIGH);
  // delay(4000);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "OK", 2);
}

static esp_err_t leftturn_handler(httpd_req_t *req) {
  // Motor(HIGH, LOW, LOW, HIGH);
  digitalWrite(vacuum, HIGH);
  Serial.println("Left");
  direction = "left";
  Motor(HIGH, LOW, HIGH, LOW);
  //delay(6000);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "OK", 2);
}
static esp_err_t rightturn_handler(httpd_req_t *req) {
  // Motor(LOW, HIGH, HIGH, LOW);

  digitalWrite(vacuum, HIGH);
  Serial.println("Right");
  direction = "right";
  Motor(LOW, HIGH, LOW, HIGH);
  //delay(6000);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "OK", 2);
}

static esp_err_t stop_handler(httpd_req_t *req) {
  Motor(LOW, LOW, LOW, LOW);
  digitalWrite(vacuum, LOW);
  Serial.println("Stop");
  direction = "stop";
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "OK", 2);
}

String ListFiles() {
  //SD Card
  if(!SD_MMC.begin()){
    Serial.println("Card Mount Failed");
    return "Card Mount Failed";
  }  
  
  fs::FS &fs = SD_MMC; 
  File root = fs.open("/");
  if(!root){
    Serial.println("Failed to open directory");
    return "Failed to open directory";
  }

  String list = "";
  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory()){
      String filename=String(file.name());
      list = "<tr><td>"+String(file.name())+"</td><td align='right'>"+String(file.size())+"</td><td><button onclick=\"location.href='/control?delete="+String(file.name())+"\';\">Delete</button></td></tr>"+list;
    }
    file = root.openNextFile();
  }
  if (list=="") list = "<tr><td>null</td><td>null</td><td></td></tr>";
  list="<table style='border: 1px solid black'><tr><th align='center'>File Name</th><th align='center'>Size</th><th></th></tr>"+list+"</table>";

  file.close();
  SD_MMC.end();

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);   
  
  return list;
}

String DeleteFile(String filename) {
  //SD Card
  if(!SD_MMC.begin()){
    Serial.println("Card Mount Failed");
    return "Card Mount Failed";
  }  
  
  fs::FS &fs = SD_MMC;
  File file = fs.open(filename);
  String message="";
  if(fs.remove(filename)){
      message=filename + " File deleted";
  } else {
      message=filename + " Delete failed";
  }
  file.close();
  SD_MMC.end();

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);   

  return message;
}

//指令參數控制
static esp_err_t cmd_handler(httpd_req_t *req){
    char*  buf;  //存取網址後帶的參數字串
    size_t buf_len;
    char variable[128] = {0,};  //存取參數var值
    char value[128] = {0,};  //存取參數val值
    String myCmd = "";

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
          if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
            httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
          } 
          else {
            myCmd = String(buf);
          }
        }
        free(buf);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    Feedback="";Command="";cmd="";P1="";P2="";P3="";P4="";P5="";P6="";P7="";P8="";P9="";
    ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;     
    if (myCmd.length()>0) {
      myCmd = "?"+myCmd;  //網址後帶的參數字串轉換成自訂參數格式
      for (int i=0;i<myCmd.length();i++) {
        getCommand(char(myCmd.charAt(i)));  //拆解參數字串
      }
    }

    if (cmd.length()>0) {
      //Serial.println("");
      //Serial.println("Command: "+Command);
      //Serial.println("cmd= "+cmd+" ,P1= "+P1+" ,P2= "+P2+" ,P3= "+P3+" ,P4= "+P4+" ,P5= "+P5+" ,P6= "+P6+" ,P7= "+P7+" ,P8= "+P8+" ,P9= "+P9);
      //Serial.println(""); 

      //自訂指令區塊  http://192.168.xxx.xxx/control?cmd=P1;P2;P3;P4;P5;P6;P7;P8;P9
      if (cmd=="record") {
          Serial.println("");
          Serial.println("Start recording");
          frame_cnt = 0;
          start_record = 1;
          Feedback="Start Record - done";
      }
      else if (cmd=="stop") { 
        start_record = 0;
        Feedback="Stop Record - done";
      }       
      else if (cmd=="delete") { 
        Feedback=DeleteFile(P1)+"<br>"+ListFiles(); 
      }  
      else if (cmd=="resetfilegroup") { 
        resetfilegroup = true;
        do_eprom_read();
        Feedback="Reset file group - done"; 
      }
      else if (cmd=="message") { 
        Feedback=recordMessage;
      }
      else if (cmd=="restart") { 
        ESP.restart();
      }  
      else {
        Feedback="Command is not defined";
      }
    
      const char *resp = Feedback.c_str();
      httpd_resp_set_type(req, "text/html");  //設定回傳資料格式
      httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");  //允許跨網域讀取
      return httpd_resp_send(req, resp, strlen(resp));    
    } 
    else {
      int val = atoi(value);
      sensor_t * s = esp_camera_sensor_get();
      int res = 0;
        
      if(!strcmp(variable, "framesize")) {
        if(s->pixformat == PIXFORMAT_JPEG) 
          res = s->set_framesize(s, (framesize_t)val);
      }
      else if(!strcmp(variable, "quality")) res = s->set_quality(s, val);
      else if(!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
      else if(!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
      else if(!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
      else if(!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
      else if(!strcmp(variable, "flash")) {
        ledcAttachPin(4, 4);  
        ledcSetup(4, 5000, 8);        
        ledcWrite(4,val);
      }
      else if(!strcmp(variable, "recordonce")) {
          recordOnce = val;        
          if (val==1) {
            Serial.println("Record once");
            Feedback="Record once - done";
          }
          else {
            Serial.println("Record continuously");
            Feedback="Record continuously - done";
          }
      }    
      else if(!strcmp(variable, "avilength")) {
        avi_length = val;
        Serial.println("avi_length = " + String(val));
        Feedback="avi_length = " + String(val);        
      }
      else {
          res = -1;
      }
  
      if(res){
          return httpd_resp_send_500(req);
      }

      if (Feedback!="") {
        const char *resp = Feedback.c_str();
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_send(req, resp, strlen(resp));  //回傳參數字串
      }
      else {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_send(req, NULL, 0);
      }    
    }
}

static esp_err_t list_handler(httpd_req_t *req) {
  String list = "";
  if (recordMessage!="Do nothing")
    list = "Get error. Recording...";
  else
    list = ListFiles();
    
  //Serial.println(list);
  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
  <html>
  <head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  </head>
  <body>
  %s
  </body>
  </html>)rawliteral";

  sprintf(the_page, msg, list.c_str());
  
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, the_page, strlen(the_page));

  return ESP_OK;
}

bool start_streaming = false;

#define PART_BOUNDARY "123456789000000000000987654321"

void the_streaming_loop (void* pvParameter);

static esp_err_t stream_handler(httpd_req_t *req) {

  long start = millis();

  Serial.print("stream_handler, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

  start_streaming = true;

  xTaskCreatePinnedToCore( the_streaming_loop, "the_streaming_loop", 8000, req, 1, &the_streaming_loop_task, 0);

  if ( the_streaming_loop_task == NULL ) {
    //vTaskDelete( xHandle );
    Serial.printf("do_the_steaming_task failed to start! %d\n", the_streaming_loop_task);
  }

  time_in_web1 += (millis() - start);

  while (start_streaming == true) {          // we have to keep the *req alive
    delay(1000);
    //Serial.print("z");
  }
  Serial.println("Streaming done");
}

void the_streaming_loop (void* pvParameter) {      
  httpd_req_t *req;
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  long start = millis();

  Serial.print("\n\nlow prio stream_handler task, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

  req = (httpd_req_t *) pvParameter;

  Serial.println("Starting the streaming");

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    Serial.printf("after first httpd_resp_set_type %d\n", res);
    start_streaming = false;
  }

  int streaming_frames = 0;
  long streaming_start = millis();

  while (true) {
    streaming_frames++;

    if (fb_curr == NULL) {
      fb = get_good_jpeg(); //esp_camera_fb_get();
      if (!fb) {
        Serial.println("Stream - Camera Capture Failed");
        start_streaming = false;
      }
      framebuffer_len = fb->len;
      memcpy(framebuffer, fb->buf, fb->len);
      esp_camera_fb_return(fb);

    } else {
      fb = fb_curr;
      framebuffer_len = fb->len;
      memcpy(framebuffer, fb->buf, fb->len);
    }

    _jpg_buf_len = framebuffer_len;
    _jpg_buf = framebuffer;

    size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
    long send_time = millis();
    
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    if (res != ESP_OK) {
      //Serial.printf("Stream error - 1st send chunk %d\n",res);
      start_streaming = false;
    }

    res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    if (res != ESP_OK) {
      //Serial.printf("Stream error - 2nd send chunk %d\n",res);
      start_streaming = false;
    }

    res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if (res != ESP_OK) {
      //Serial.printf("Stream error - 3rd send chunk %d\n", res);
      start_streaming = false;
    }

    if (millis() - send_time > stream_delay) {
      stream_delay = stream_delay * 1.5;
    }

    time_in_web2 += (millis() - start);

    if (streaming_frames % 100 == 10) {
      if (Lots_of_Stats) Serial.printf("Streaming at %3.3f fps\n", (float)1000 * streaming_frames / (millis() - streaming_start));
    }

    delay(stream_delay);
    start = millis();

    if (start_streaming == false) {
      Serial.println("Deleting the streaming task");
      delay(100);
      vTaskDelete(the_streaming_loop_task);
    }
  }  // stream forever
}



void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t forward_uri = {
    .uri       = "/forward",
    .method    = HTTP_GET,
    .handler   = forward_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t reverse_uri = {
    .uri       = "/reverse",
    .method    = HTTP_GET,
    .handler   = reverse_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stop_uri = {
    .uri       = "/stop",
    .method    = HTTP_GET,
    .handler   = stop_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t leftturn_uri = {
    .uri       = "/leftturn",
    .method    = HTTP_GET,
    .handler   = leftturn_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t rightturn_uri = {
    .uri       = "/rightturn",
    .method    = HTTP_GET,
    .handler   = rightturn_handler,
    .user_ctx  = NULL
  };


  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t cmd_uri = {
    .uri       = "/control",
    .method    = HTTP_GET,
    .handler   = cmd_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = capture_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t list_uri = {
    .uri       = "/list",
    .method    = HTTP_GET,
    .handler   = list_handler,
    .user_ctx  = NULL
  };


  ra_filter_init(&ra_filter, 20);
  Serial.printf("Starting web server on port: '%d'", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &forward_uri);
    httpd_register_uri_handler(camera_httpd, &reverse_uri);
    httpd_register_uri_handler(camera_httpd, &stop_uri);
    httpd_register_uri_handler(camera_httpd, &leftturn_uri);
    httpd_register_uri_handler(camera_httpd, &rightturn_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &stream_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);   
    httpd_register_uri_handler(camera_httpd, &list_uri);   

  }

  config.server_port += 1;
  config.ctrl_port += 1;
  Serial.printf("Starting stream server on port: '%d'", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
    Serial.println("Camera http started");
}

void Motor(int L1, int L2, int R1, int R2)
{
  digitalWrite(leftmotor1, L1);
  digitalWrite(leftmotor2, L2);
  digitalWrite(rightmotor1, R1);
  digitalWrite(rightmotor2, R2);
}
