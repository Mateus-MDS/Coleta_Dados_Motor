import numpy as np
import matplotlib.pyplot as plt
import os

# Caminho absoluto do arquivo de dados, relativo ao script
script_dir = os.path.dirname(os.path.abspath(__file__))
data_path = os.path.join(script_dir, 'tarefa1.csv')
data = np.loadtxt(data_path, delimiter=',')

amostra = data[:, 0]
accel_x = data[:, 1]
accel_y = data[:, 2]
accel_z = data[:, 3]
giro_x = data[:, 4]
giro_y = data[:, 5]
giro_z = data[:, 6]

# Cria vetor de tempo (10000 amostras, 0.1s entre cada)
tempo = amostra * 0.1  # ou np.arange(len(amostra)) * 0.1

fig, axs = plt.subplots(3, 2, figsize=(12, 8))

axs[0, 0].plot(tempo, accel_x, 'b')
axs[0, 0].set_title('Acelerômetro X')
axs[0, 0].set_xlabel('Tempo (s)')
axs[0, 0].set_ylabel('Aceleração X (m/s²)')
axs[0, 0].grid()

axs[1, 0].plot(tempo, accel_y, 'g')
axs[1, 0].set_title('Acelerômetro Y')
axs[1, 0].set_xlabel('Tempo (s)')
axs[1, 0].set_ylabel('Aceleração Y (m/s²)')
axs[1, 0].grid()

axs[2, 0].plot(tempo, accel_z, 'r')
axs[2, 0].set_title('Acelerômetro Z')
axs[2, 0].set_xlabel('Tempo (s)')
axs[2, 0].set_ylabel('Aceleração Z (m/s²)')
axs[2, 0].grid()

axs[0, 1].plot(tempo, giro_x, 'c')
axs[0, 1].set_title('Giroscópio X')
axs[0, 1].set_xlabel('Tempo (s)')
axs[0, 1].set_ylabel('Giro X (°/s)')
axs[0, 1].grid()

axs[1, 1].plot(tempo, giro_y, 'm')
axs[1, 1].set_title('Giroscópio Y')
axs[1, 1].set_xlabel('Tempo (s)')
axs[1, 1].set_ylabel('Giro Y (°/s)')
axs[1, 1].grid()

axs[2, 1].plot(tempo, giro_z, 'y')
axs[2, 1].set_title('Giroscópio Z')
axs[2, 1].set_xlabel('Tempo (s)')
axs[2, 1].set_ylabel('Giro Z (°/s)')
axs[2, 1].grid()

plt.tight_layout()
plt.show()