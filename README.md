USE GUIDE

Step 1: Build
    mkdir build && cd build && cmake .. && make

Step 2: Run
    Create 3 terminal an run as follow
  + Terminal 1: cd build && ./gnodeb
        Thực hiện chạy server của gnodeb, broadcast MIB và sẵn sàng nhận Paging request từ AMF
  + Terminal 2: cd build && ./ue
        Thực hiện khởi động UE, đồng bộ MIB với gNodeB và giả lập wakeup để nhận Paging theo thời gian
  + Terminal 3: cd build && ./amf
        Thực hiện giả lập quá trình gửi gói tin NgAP cho gNodeB với thông tin UE default (định nghĩa ở trong common.h)

