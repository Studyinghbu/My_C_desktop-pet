# 玉桂狗桌宠 (Cinnamoroll Desk Pet)

一个用C语言编写的可爱玉桂狗桌宠程序，能够监测你的打字速度并根据速度变化表情和颜色。

## 功能特性

1. **打字速度监测**：实时记录打字数量，每1秒计算一次打字速度（WPM）
2. **动态表情变化**：
   - 快速打字（>60 WPM）：严肃表情，红色调
   - 慢速打字（<20 WPM）：可爱表情，粉色调，带有腮红
   - 正常速度（20-60 WPM）：正常表情，白色调
3. **平滑过渡效果**：表情和颜色变化自然平滑
4. **鼠标交互**：可以通过鼠标拖动桌宠到任意位置
5. **桌面运行**：直接在Windows桌面运行，无需浏览器

## 项目结构

```
c_desk/
├── src/
│   ├── main.c          # 主程序文件
│   └── create_bmp.c    # BMP图片生成工具
├── res/
│   ├── normal.bmp      # 正常表情图片
│   ├── serious.bmp     # 严肃表情图片
│   └── cute.bmp        # 可爱表情图片
├── bin/
│   ├── cinnamoroll_pet.exe  # 桌宠可执行文件
│   └── create_bmp.exe       # 图片生成工具可执行文件
├── Makefile            # 编译脚本
└── README.md           # 项目说明
```

## 编译和运行

### 方法一：使用Makefile（推荐）

1. 确保安装了MinGW或其他C语言编译器
2. 在项目根目录执行以下命令：
   ```bash
   make all
   make resources
   ```
3. 运行桌宠程序：
   ```bash
   ./bin/cinnamoroll_pet.exe
   ```

### 方法二：手动编译

1. 编译BMP图片生成工具：
   ```bash
   gcc -o bin/create_bmp.exe src/create_bmp.c -luser32 -lgdi32
   ```

2. 生成图片资源：
   ```bash
   cd bin
   ./create_bmp.exe
   ```

3. 编译桌宠主程序：
   ```bash
   cd ..
   gcc -o bin/cinnamoroll_pet.exe src/main.c -luser32 -lgdi32
   ```

4. 运行桌宠程序：
   ```bash
   cd bin
   ./cinnamoroll_pet.exe
   ```

## 使用说明

1. 启动程序后，玉桂狗桌宠会显示在桌面上
2. 可以通过鼠标左键拖动桌宠到任意位置
3. 开始打字后，桌宠会根据你的打字速度自动改变表情和颜色
4. 按ESC键可以关闭桌宠程序

## 技术实现

- 使用Win32 API创建桌面窗口程序
- 实现键盘钩子监测打字输入
- 使用内存DC绘制和切换图片
- 基于打字速度的表情平滑过渡算法
- 支持透明背景和窗口拖动

## 自定义图片

如果想使用自己的玉桂狗图片，可以替换`res`目录下的三个BMP文件：
- `normal.bmp`：正常表情
- `serious.bmp`：严肃表情
- `cute.bmp`：可爱表情

图片尺寸建议为200x200像素，背景色使用白色（会被设置为透明）。

## 注意事项

1. 程序需要在Windows系统上运行
2. 确保编译器支持Win32 API
3. 如果图片加载失败，请检查图片路径是否正确
4. 按ESC键可以快速退出程序

## 许可证

本项目采用MIT许可证，可自由使用和修改。
