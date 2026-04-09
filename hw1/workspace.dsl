workspace "Event Management System" "Архитектура системы управления событиями (Вариант 22)" {

    model {
        user = person "Пользователь" "Организатор событий или участник."
        
        emailSystem = softwareSystem "Email-сервис" "Отправка электронных билетов." "External"
        paymentSystem = softwareSystem "Платежный шлюз" "Обработка транзакций." "External"

        eventSystem = softwareSystem "Система управления событиями" "Создание мероприятий, поиск, продажа билетов." {
            
            webApp = container "Web Application" "Клиентское приложение" "React / TypeScript" "Web Browser"
            apiGateway = container "API Gateway" "Маршрутизация HTTP -> gRPC" "Nginx"
            
            searchService = container "Search Service" "Полнотекстовый поиск." "c++ (userver) / gRPC"
            userService = container "User Service" "Управление профилями." "c++ (userver) / gRPC"
            eventService = container "Event Service" "Каталог и учет мест." "c++ (userver) / gRPC"
            bookService = container "Book Service" "Бронирование и платежи." "c++ (userver) / gRPC"

            redisCache = container "Event Cache" "In-memory хранилище." "Redis" "Database"
            elasticSearch = container "Elasticsearch" "Поисковый индекс." "Elasticsearch" "Database"
            database = container "Database" "Единая реляционная СУБД." "PostgreSQL" "Database"
        }

        user -> webApp "Использует интерфейс"
        webApp -> apiGateway "REST API вызовы" "HTTPS/JSON"
        
        apiGateway -> searchService "route: /api/search" "gRPC"
        apiGateway -> userService "route: /api/users" "gRPC"
        apiGateway -> eventService "route: /api/events" "gRPC"
        apiGateway -> bookService "route: /api/bookings" "gRPC"
        
        searchService -> elasticSearch "Поисковые запросы" "TCP"
        eventService -> elasticSearch "Индексация" "HTTP"
        eventService -> redisCache "Чтение/запись" "TCP"
        
        userService -> database "CRUD пользователей" "TCP"
        eventService -> database "Транзакционное управление местами" "TCP"
        bookService -> database "Управление статусом брони" "TCP"

        bookService -> userService "RPC запрос профиля" "gRPC"
        bookService -> eventService "RPC проверка мест" "gRPC"
        
        bookService -> paymentSystem "Эквайринг" "HTTPS/REST"
        bookService -> emailSystem "Передача билета" "HTTPS/REST"
    }

    views {
        systemContext eventSystem "SystemContext" {
            include *
            autoLayout tb
            description "Системный контекст платформы."
        }

        container eventSystem "Containers" {
            include *
            autoLayout tb 300 300
            description "Межсервисная архитектура системы (C2)."
        }

        dynamic eventSystem "CompleteFlow" "Полный цикл обработки заказа (Все этапы)" {
            description "Хронологическая последовательность: [Поиск] -> [Выбор мест] -> [Оплата и Нотификации]."
            
            user -> webApp "1. [Поиск] Вводит поисковый запрос"
            webApp -> apiGateway "2. [Поиск] GET /search"
            apiGateway -> searchService "3. [Поиск] RPC SearchEvents"
            searchService -> elasticSearch "4. [Поиск] Поиск события в индексе"
            
            user -> webApp "5. [Выбор] Открывает карточку события"
            webApp -> apiGateway "6. [Выбор] GET /events/{id}"
            apiGateway -> eventService "7. [Выбор] RPC GetEventDetails"
            eventService -> redisCache "8. [Выбор] Чтение описания из кэша"
            eventService -> database "9. [Выбор] Чтение свободных мест из БД"
            
            user -> webApp "10. [Оплата] Жмет 'Оплатить'"
            webApp -> apiGateway "11. [Оплата] POST /bookings"
            apiGateway -> bookService "12. [Оплата] RPC CreateBooking"
            
            bookService -> eventService "13. [Оплата] RPC запрос на фиксацию мест"
            eventService -> database "14. [Оплата] Блокировка мест"
            
            bookService -> database "15. [Оплата] Сохранение брони (Ожидание)"
            bookService -> paymentSystem "16. [Оплата] Проведение эквайринга"
            bookService -> database "17. [Оплата] Обновление статуса (Успешно)"
            
            bookService -> emailSystem "18. [Оплата] Отправка Email-билета"
            
            autoLayout lr 400 300
        }

        styles {
            element "Software System" {
                background #1168bd
                color #ffffff
            }
            element "External" {
                background #999999
                color #ffffff
            }
            element "Person" {
                shape person
                background #08427b
                color #ffffff
            }
            element "Container" {
                background #438dd5
                color #ffffff
            }
            element "Web Browser" {
                shape WebBrowser
            }
            element "Database" {
                shape Cylinder
            }
        }
    }
}
