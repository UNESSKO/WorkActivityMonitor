from django.db import models

class Client(models.Model):
    domain = models.CharField(max_length=100)
    machine_name = models.CharField(max_length=100)
    ip_address = models.GenericIPAddressField(unique=True)
    username = models.CharField(max_length=100)
    last_active = models.DateTimeField()
    is_connected = models.BooleanField(default=True)
    screenshot = models.TextField(blank=True, null=True)

    def __str__(self):
        return f"{self.machine_name} ({self.ip_address})"
