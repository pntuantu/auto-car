# 1. Khai báo phiên bản CMake tối thiểu
cmake_minimum_required(VERSION 3.16)

# 2. Khai báo các thư mục chứa thư viện ngoài (Lưu ý: thư mục 'main' thường được ESP-IDF tự động nhận diện nên thực ra bạn bỏ dòng này đi cũng được, nhưng để lại không sao)
set(EXTRA_COMPONENT_DIRS main)

# 3. Nạp bộ toolchain biên dịch chéo của ESP-IDF (BẮT BUỘC PHẢI ĐỨNG TRƯỚC LỆNH PROJECT)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 4. Khởi tạo project (BẮT BUỘC ĐỨNG CUỐI CÙNG)
project(auto-car)