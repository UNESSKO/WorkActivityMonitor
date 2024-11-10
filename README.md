# WorkActivityMonitor

## Описание
`WorkActivityMonitor` — это программа для мониторинга активности пользователя на рабочем компьютере. Она состоит из двух компонентов:

- **Клиент** — собирает информацию о машине, пользователе и делает скриншоты экрана, затем отправляет эти данные на сервер.
- **Сервер** — принимает данные от клиентов, сохраняет их и предоставляет доступ к скриншотам через веб-интерфейс.

## Установка

### Установка клиента

- g++ client.cpp -o WorkActivityMonitor -lws2_32 -lgdi32 -mwindows

### Установка сервера

- python -m venv venv
- .\venv\Scripts\activate
- pip install -r requirements.txt
- python manage.py migrate
- python manage.py runserver
