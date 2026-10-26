# Кумир 2 (Порт на Qt6) + DOOM I Raycaster Engine

Форк системы программирования «Кумир 2» (оригинальный репозиторий: `https://github.com/a-a-maly/kumir2.git`).

## 1. Что сделано в этом форке

### Портирование на современный стек Qt6 / C++17
- Полный перенос кодовой базы со старого Qt4 / Qt5 на **Qt6** (`Qt6Widgets`, `Qt6Core`, `Qt6Gui`, `Qt6Svg`, `Qt6Xml`).
- Реализован заголовочный слой совместимости `include/kumir2_qt6_compat.h` (`QRegularExpression`, замена `trUtf8`, `endl`, `pos()`, `QWheelEvent`).
- Исправлена сборка всех библиотек (`libKStdlib.so`, `libVM.so`, `libExtensionSystem.so`, `libWidgets.so`, `libDataFormats.so`), исполнителей (`ActorRobot`, `ActorKeyboard`, `ActorPainter`, `ActorDraw`, `ActorTurtle`, `ActorVodoley`, `ActorGrasshopper`, `ActorComplexNumbers`, `Actor_Colorer`) и приложений (`kumir2-classic`, `kumir2-ide`, `kumir2-run`, `kumir2-xrun`, `kumir2-bc`).

### Ликвидация 100% утечек памяти (Valgrind Memcheck Clean)
- Устранены скрытые утечки в `RobotModule`, `KumirCompilerTool`, `KumirCodeRun::Run` (деструкторы мьютексов отладки).
- Устранены `out-of-bounds` чтения в `KumirVM::getMainModuleGlobals` и `KumirVM::do_call`.
- Прогон под **Valgrind Memcheck** подтверждает **0 байт утечек**:
  ```text
  LEAK SUMMARY:
     definitely lost: 0 bytes in 0 blocks
     indirectly lost: 0 bytes in 0 blocks
       possibly lost: 0 bytes in 0 blocks
     still reachable: 314,787 bytes in 126 blocks
  ERROR SUMMARY: 0 errors from 0 contexts
  ```

### Оптимизация исполнителей и I/O
- Исправлен `IO::InputStream` в `kfiles.cpp`: корректное чтение сырых бинарных файлов без падения парсера UTF-8.
- Убраны паразитные `qDebug()` и искусственные задержки `msleep` в исполнителе Робот: скорость обработки шагов выросла до **~20 000 шагов/сек**.
- Увеличены лимиты сетки поля Робота до 1024x1024.
- Добавлена глобальная обработка ввода в `ActorKeyboard`.

---

## 2. DOOM I 3D Engine на чистом Кумире

Весь движок реализован **на 100% на языке Кумир** (файлы `doom_interactive.kum`, `doom_engine.kum`), без каких-либо специализированных хаков в C++:
- **Парсинг WAD**: чтение бинарного формата IWAD (сектора стартовой зоны E1M1) напрямую через модуль `Файлы`.
- **DDA Raycaster**: трассировка лучей Digital Differential Analysis с математическим расчетом перпендикулярного расстояния до стен и перспективы без искажений.
- **Рендеринг Роботом**: динамическая отрисовка кадра на поле исполнителя Робот.
- **Интерактивное управление**: WASD / Стрелки (движение и поворот в реальном времени), Пробел (выход).

---

## 3. Сборка и запуск

### Требования
- CMake >= 3.16
- GCC / Clang с поддержкой C++17
- Qt6 (`qt6-base-dev`, `libqt6svg6-dev`)
- Ninja / Make

### Сборка
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Запуск интерактивного DOOM
```bash
# Консольный/автономный запуск с полем 32x20
./build/bin/kumir2-xrun --field=field32.fil doom_interactive.kod

# Или открытие в IDE:
./build/bin/kumir2-classic doom_interactive.kum
```
