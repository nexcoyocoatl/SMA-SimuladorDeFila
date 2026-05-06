Para compilação, é necessário ter instalado Make e o compilador GCC.  
Execute o comando "make" para compilar e "./simulate_queue <nome_do_arquivo_de_config.yml>" para rodar. "simulate_queue.exe <nome_do_arquivo_de_config.yml>" no Windows.  
Caso não exista o arquivo de config model.yml, será criado um padrão para rodar o programa, baseado no T1. Caso seja enviado um parâmetro de um arquivo que não exista, também cria o arquivo padrão e o utiliza.  
Testado apenas em Linux e Windows.  
  
Parâmetros do arquivo de configuração:  
arrivals -> Fila(s) das quais chegam as unidades, com um determinado tempo na simulação  
queues -> Parâmetros das filas (servers, capacity, minArrival, maxArrival, minService, maxService). Caso não exista o parâmetro capacity, a fila será infinita.  
network -> Cria as rotas entre filas durante atendimento e consequente saída, com as probabilidades para cada rota determinadas por número aleatório.  
maxrndnumbers -> Determina número máximo de geração de números aleatórios até que seja finalizada a simulação.  
  
Exemplo do arquivo de configuração de filas:  
```
arrivals:  
   Q1: 2.0  
  
queues:  
   Q1:  
      servers: 1  
      minArrival: 2.0  
      maxArrival: 4.0  
      minService: 1.0  
      maxService: 2.0  
   Q2:  
      servers: 2  
      capacity: 5  
      minService: 4.0  
      maxService: 6.0  
   Q3:  
      servers: 2  
      capacity: 10  
      minService: 5.0  
      maxService: 15.0  
  
network:  
-  source: Q1  
   target: Q2  
   probability: 0.8  
-  source: Q1  
   target: Q3  
   probability: 0.2  
-  source: Q2  
   target: Q1  
   probability: 0.3  
-  source: Q2  
   target: Q2  
   probability: 0.5  
-  source: Q3  
   target: Q3  
   probability: 0.7  
  
maxrndnumbers: 100000
```
