Sustav se sastoji od tri servisa. Sensor servis generira očitanja temperature i vlage te ih šalje na rabbitmq queue sensor.data.
Control čita očitanja sa sensor.data te šalje notifikacije na alert koji ih ispisuje.
Sensor servis se može srušiti ako je visoka temperatura i visoka vlaga, a docker će ponovo pokrenuti kontejnjer.