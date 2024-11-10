from django.contrib import admin
from .models import Client

class ClientAdmin(admin.ModelAdmin):
    list_display = ('domain', 'machine_name', 'ip_address', 'username', 'last_active')
    search_fields = ('domain', 'machine_name', 'ip_address', 'username')
    list_filter = ('last_active',)

# Регистрируем модель в админке
admin.site.register(Client, ClientAdmin)
