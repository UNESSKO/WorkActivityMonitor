import os
import json
import base64
from .models import Client
from datetime import datetime
from django.conf import settings
from django.utils import timezone
from django.shortcuts import render
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt

@csrf_exempt
def index(request):
    if request.method == 'POST':
        # Читаем тело запроса как JSON
        try:
            data = json.loads(request.body)
            domain = data.get('Domain', '')
            machine = data.get('Machine Name', '')
            ip = data.get('IP Address', '')
            user = data.get('User', '')
            last_active_str = data.get('Last Active', '')
            screenshot_base64 = data.get('Screenshot', '')

            # Преобразуем строку в дату
            try:
                last_active = datetime.strptime(last_active_str, "%a %b %d %H:%M:%S %Y")
                #last_active = timezone.localtime(last_active)
            except ValueError:
                return JsonResponse({'error': 'Invalid date format'}, status=400)

            # Преобразуем Base64 изображение в файл
            screenshot_url = None
            if screenshot_base64:
                # Декодируем Base64 строку
                image_data = base64.b64decode(screenshot_base64.split(',')[1])
                file_name = f"{machine}_screenshot.png"  # Имя файла
                file_path = os.path.join(settings.MEDIA_ROOT, "screenshots", file_name)

                # Создаем папку, если она не существует
                if not os.path.exists(os.path.dirname(file_path)):
                    os.makedirs(os.path.dirname(file_path))

                # Сохраняем изображение в файл
                with open(file_path, 'wb') as f:
                    f.write(image_data)

                screenshot_url = f"/media/screenshots/{file_name}"  # Ссылка на изображение

            # Обновление или создание клиента
            client, created = Client.objects.update_or_create(
                username = user,
                defaults={
                    "domain": domain,
                    "ip_address": ip,
                    "machine_name": machine,
                    "last_active": last_active,
                    "is_connected": True,
                    "screenshot": screenshot_url,
                },
            )

            return JsonResponse({'status': 'success'}, status=200)

        except json.JSONDecodeError:
            return JsonResponse({'error': 'Invalid JSON'}, status=400)

    now = timezone.now()
    timeout = timezone.timedelta(minutes=10)
    Client.objects.filter(last_active__lt=now - timeout).update(is_connected=False)

    clients = Client.objects.all()
    return render(request, 'index.html', {'clients': clients})
