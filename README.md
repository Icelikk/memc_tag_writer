Memcached Benchmark + PostgreSQL 

 Создан для нагрузочного тестирования Memcached и автоматической выгрузки данных в PostgreSQL. 
 Измеряет производительность записи пакетов (id, v, q, t) и скорость массовой вставки в базу.


**Требования:** Docker Engine от 20.10, Docker Compose от 2.0, Git.

Порядок запуска:
```bash
git clone https://github.com/Icelikk/memc_tag
cd memcached-benchmark
docker-compose up -d
docker exec -it memcached_app bash
cd /app
./run_test.sh
```
Доступно также тестирование без выгрузки в Postgres, по флагу --without-pg
```bash
./run_test.sh --without-pg
```
Результаты появятся на хосте в папке ./results/ (CSV-файлы)

## Структура проекта

```
.
├── docker-compose.yml       
├── Dockerfile               
├── CMakeLists.txt           # Сборка memc_writer и pg_memc
├── memc_writer.cpp          # Бенчмарк записи в Memcached
├── pg_memc.cpp              # Выгрузка из Memcached в PostgreSQL
├── run_test.sh              # Скрипт запуска тестов
├── init.sql                 # Инициализация схемы PostgreSQL
├── third_party/
│   └── plog/                # Библиотека логирования
└── results/                 # CSV с результатами (монтируется на хост)
```

---

1. **memc_writer** — записывает пакеты данных в Memcached с заданными периодом и размером.  
2. **pg_memc** — читает данные из Memcached и выполняет массовую вставку (`COPY`) в PostgreSQL.  
3. **run_test.sh** — скрипт запуска , собирает метрики и сохраняет CSV.

---

## Управление контейнерами

**Перезапуск и пересборка:**
(При изменении кода обязательно пересобирайте проект)
```bash
docker-compose down -v
docker-compose build --no-cache
docker-compose up -d
```

**Ручная выгрузка данных из Memcached в PostgreSQL:**

```bash
docker exec -it memcached_app /usr/local/bin/memcached_to_pg <число_пакетов>
```
---
## Параметры тестирования

Все параметры задаются в `run_test.sh`:

| Параметр     | Описание                                         | Пример         |
|--------------|--------------------------------------------------|----------------|
| `PERIODS`    | это период, с которым программа записывает пакеты данных в Memcached                      | `(10 50 100)`  |
| `TAGS_LIST`  | Размеры пакетов (количество записей за цикл)     | `(100 500 1000)` |
| `DURATION`   | Длительность одного теста (сек)                  | `60`           |
| `THRESHOLD`  | Допустимое количество превышений периода         | `5`  
<img width="270" height="109" alt="изображение" src="https://github.com/user-attachments/assets/3411f144-6e68-4fda-8fd0-cea92742031f" />

Чем меньше период, тем выше нагрузка на систему. Если программа не успевает обработать пакет за указанный период, в логе появляется запись "Превышение времени на ...". Превышения подсчитываются и учитываются в exceed_count. Если их становится больше THRESHOLD, тест для данного периода прекращается, а максимальный успешный размер пакета запоминается
