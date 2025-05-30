import numpy as np
import json
import torch
import torch.nn as nn
from torch.autograd import Variable
from sklearn.preprocessing import StandardScaler, MinMaxScaler
import os
from datetime import datetime

def get_all_input_files():
    input_dir = "/app/data/parser_input"
    files = [os.path.join(input_dir, f) for f in os.listdir(input_dir) 
             if os.path.isfile(os.path.join(input_dir, f))]
    files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    return files

input_files = get_all_input_files()
for input_file in input_files:
  with open(input_file) as f:
    data = json.load(f)

  data_train_x = np.array(
      [(float(item['open']), 
        float(item['high']), 
        float(item['low']), 
        float(item['volume'])) 
      for item in data]
  )



  data_train_y = np.array(
      [( 
        float(item['close'])) 
      for item in data]
  )


  mm = MinMaxScaler()
  ss = StandardScaler()


  X_ss = ss.fit_transform(data_train_x)
  y_mm = mm.fit_transform(data_train_y.reshape(-1, 1))

  X_train_tensors = Variable(torch.Tensor(X_ss))

  y_train_tensors = Variable(torch.Tensor(y_mm))

  X_train_tensors_final = torch.reshape(X_train_tensors,   (X_train_tensors.shape[0], 1, X_train_tensors.shape[1]))

  class LSTM1(nn.Module):
      def __init__(self, num_classes, input_size, hidden_size, num_layers, seq_length):
          super(LSTM1, self).__init__()
          self.num_classes = num_classes
          self.num_layers = num_layers
          self.input_size = input_size
          self.hidden_size = hidden_size
          self.seq_length = seq_length

          self.lstm = nn.LSTM(input_size=input_size, hidden_size=hidden_size,
                            num_layers=num_layers, batch_first=True)
          self.fc_1 =  nn.Linear(hidden_size, 128)
          self.fc = nn.Linear(128, num_classes)

          self.relu = nn.ReLU()

      def forward(self,x):
          h_0 = Variable(torch.zeros(self.num_layers, x.size(0), self.hidden_size))
          c_0 = Variable(torch.zeros(self.num_layers, x.size(0), self.hidden_size))

          output, (hn, cn) = self.lstm(x, (h_0, c_0))
          hn = hn.view(-1, self.hidden_size)
          out = self.relu(hn)
          out = self.fc_1(out)
          out = self.relu(out)
          out = self.fc(out)
          return out

  num_epochs = 1000
  learning_rate = 0.001

  input_size = 4
  hidden_size = 50
  num_layers = 1

  num_classes = 1

  lstm1 = None

  lstm1 = LSTM1(num_classes, input_size, hidden_size, num_layers, X_train_tensors_final.shape[1])

  criterion = torch.nn.MSELoss()
  optimizer = torch.optim.Adam(lstm1.parameters(), lr=learning_rate)


  for epoch in range(num_epochs):
    outputs = lstm1.forward(X_train_tensors_final)
    optimizer.zero_grad()

    loss = criterion(outputs, y_train_tensors)

    loss.backward()

    optimizer.step()


  train_predict = lstm1(X_train_tensors_final)
  data_predict = train_predict.data.numpy()[:240]

  data_predict = mm.inverse_transform(data_predict)


  def save_predictions(predictions):
      os.makedirs("/app/data/model_output", exist_ok=True)

      arr = input_file.split("/")
      output_path = f"/app/data/model_output/predictions_{arr[-1]}"
      
      with open(output_path, 'w') as f:
          json.dump({"predictions": predictions.tolist()}, f)
      
      return output_path



  prediction_file = save_predictions(data_predict)
  print(prediction_file)