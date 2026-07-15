# Cache L1 com Substituição Baseada em Perceptron (AIRA)

Este repositório contém a implementação, simulação e análise de desempenho de uma **Memória Cache L1 de 2 vias (Set-Associative)** com um mecanismo de substituição inteligente baseado em **Perceptron (AIRA)**, comparado ao algoritmo clássico de **LRU (Least Recently Used)**.

O projeto une o design de hardware em **Verilog (RTL)**, modelagem visual no **Logisim** e testes de desempenho utilizando **Benchmarks em C**.

---

## 🛠️ Estrutura do Projeto

O repositório está organizado da seguinte forma:

```text
├── docs/                     # Documentação de modelagem e esquemáticos
│   ├── PerceptronWorking.circ      # Modelos de treinamento e predição no Logisim
    ├── PerceptronWithTrainPredictionAndNear-train(xnor).circ      # Modelos de treinamento e predição no Logisim 
│   ├── mapeamentoPerceptron.pdf
│   └── MapeamentoBitsPerceptron.pdf
│
├── rtl/                      # Código fonte em Verilog (Hardware)
│   └── cache/
│       ├── aira_controller_2vias.v       # Controlador AIRA de 2 vias
│       ├── aira_replacement_controller.v # Controlador AIRA de 4 vias
│       ├── cache_l1.v                    # Módulo principal da Cache L1
│       └── replacement_lru.v             # baseline LRU
│
├── sim/                      # Ambiente de simulação de hardware
│   └── tb_cache_l1.v         # Testbench principal para a Cache L1 (Perceptron e LRU)
│
└── software/                 # Benchmarks e validação de software
    └── benchmark_c/          # Implementação em C para teste e comparação