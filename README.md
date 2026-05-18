Memcached Benchmark + PostgreSQL 

 Создан для нагрузочного тестирования Memcached и автоматической выгрузки данных в PostgreSQL. 
 Измеряет производительность записи пакетов (id, v, q, t) и скорость массовой вставки в базу.


**Требования:** Docker Engine от 20.10, Docker Compose от 2.0, Git.

Порядок запуска:
```bash
# Клонирование репозитория
git clone https://github.com/ваш-аккаунт/memcached-benchmark.git
cd memcached-benchmark

# Запуск в режиме разработки
docker-compose up -d dev

# Подключение к контейнеру
docker exec -it dev-memcached bash

# Внутри контейнера: сборка проекта
cd /app
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Запуск тестирования
./scripts/run_test.sh
```
Доступно также тестирование без выгрузки в Postgres, по флагу --without-pg
```bash
./run_test.sh --without-pg
```
Результаты появятся на хосте в папке ./results/ (CSV-файлы)
---
## 📁 Структура проекта

```
memc_tag_writer/
├── README.md                               
├── docker-compose.yml           
│
├── src/                        
│   ├── memc_writer.cpp         
│   └── pg_memc.cpp               
│
├── docker/                       
│   ├── Dockerfile                
│   └── Dockerfile.dev           
│
├── scripts/                      
│   └── run_test.sh              
│
├── config/                       
│   └── init.sql                 
│
├── build/                        
│   ├── memcached_writer          
│   ├── memcached_to_pg           
│   └── MemcachedWriter.log     
│
├── results/                     
│   ├── results_memcached.csv     
│   ├── optimization_results_memcached.csv
│   └── *.log                    

```

---

1. **memc_writer** — записывает пакеты данных в Memcached с заданными периодом и размером.  
2. **pg_memc** — читает данные из Memcached и выполняет массовую вставку (`COPY`) в PostgreSQL.  
3. **run_test.sh** — скрипт запуска , собирает метрики и сохраняет CSV.

---

### Основные команды

```bash
# Запуск всех сервисов
docker-compose up -d

# Остановка
docker-compose down

# Пересборка образов
docker-compose build --no-cache

# Просмотр логов
docker-compose logs -f dev
docker-compose logs -f memcached
docker-compose logs -f postgres-memc

# Подключение к контейнеру
docker exec -it dev-memcached bash

# Очистка volumes (удаляет все данные!)
docker-compose down -v
```

### Очистка Memcached

```bash
# Через memcached-tool
docker exec -it dev-memcached memcached-tool memcached:11211 flush

# Или через telnet
docker exec -it dev-memcached bash
telnet memcached 11211
> flush_all
> quit
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
