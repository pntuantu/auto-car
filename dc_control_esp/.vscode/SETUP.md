# Local Development Setup Guide

## Cấu hình máy để biên dịch (Machine-Specific Configuration)

Mỗi máy phát triển cần có file `c_cpp_properties.json` riêng do đường dẫn công cụ ESP-IDF khác nhau trên từng máy.

### Cách setup:

1. **Sao chép file template:**
   ```bash
   cp .vscode/c_cpp_properties.json.example .vscode/c_cpp_properties.json
   ```

2. **Cập nhật đường dẫn compiler của máy bạn:**
   - Tìm đường dẫn thực tế: `where xtensa-esp32s3-elf-gcc` (hoặc tìm trong `.espressif`)
   - Mở `c_cpp_properties.json` và cập nhật `compilerPath`

3. **File này sẽ KHÔNG được push lên GitHub** (đã thêm vào `.gitignore`)

### Ví dụ:
```json
{
  "configurations": [
    {
      "name": "ESP-IDF",
      "compilerPath": "C:\\Users\\YOUR_USERNAME\\.espressif\\tools\\xtensa-esp-elf\\esp-14.2.0_20241119\\xtensa-esp-elf\\bin\\xtensa-esp32s3-elf-gcc.exe",
      "compileCommands": "${config:idf.buildPath}/compile_commands.json",
      "includePath": [
        "${workspaceFolder}/**"
      ],
      "browse": {
        "path": [
          "${workspaceFolder}"
        ],
        "limitSymbolsToIncludedHeaders": true
      }
    }
  ],
  "version": 4
}
```

### Kết quả:
- ✅ Mỗi máy có cấu hình riêng
- ✅ GitHub sẽ không bị ảnh hưởng
- ✅ Biên dịch được trên tất cả máy
- ✅ Team members tự setup màu hơi khác nhau được
