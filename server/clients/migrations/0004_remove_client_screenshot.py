# Generated by Django 4.2.16 on 2024-11-09 17:30

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('clients', '0003_client_screenshot'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='client',
            name='screenshot',
        ),
    ]
