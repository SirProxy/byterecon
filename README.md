
# ByteRecon

ByteRecon é uma ferramenta de força bruta de arquivos e diretórios em websites, de código aberto, totalmente escrita em C. Com essa ferramenta, é possível encontrar diretórios e arquivos em websites de forma rápida e eficiente.

![ByteRecon](https://raw.githubusercontent.com/SirProxy/byterecon/main/images/pic001.png)
![ByteRecon](https://raw.githubusercontent.com/SirProxy/byterecon/main/images/pic003.png)

## Instalação

Você pode baixar o ByteRecon clonando o repositório Git:

    git clone --depth 1 https://github.com/SirProxy/byterecon.git byterecon-dev

O ByteRecon funciona utilizando a linguagem C, ou sejá será necessário compila-lo utilizando o GCC ou o compilador de sua escolha.

Após o download utilize o seguinte comando para compilar o projeto:

    gcc byterecon-dev.c -o byterecon -lpthread

### Permissão de Execução

Nos sistemas operacionais Linux, será necessário atribuir a permissão de execução para a ferramenta após a compilação. Para isso você pode utilizar:

    chmod +x byterecon

## Como usar

Para obter uma lista de opções básicas, utilize o comando:

    ./byterecon --help

Você pode realizar uma enumeração de diretórios/arquivos de forma simples, com o seguinte comando:

    ./byterecon --domain=example.com --wordlist=wordlist.txt

![ByteRecon](https://raw.githubusercontent.com/SirProxy/byterecon/main/images/pic002.png)

## Changelog v1.1.0
- Correction of URLS redirect bug.
- Adjustments in the test visualization
- Parameter Addition `--type=<dir|sub|all>`
- Parameter Addition `--ignore404`
- Addition of subdomains enumeration

## Autor

- [SirProxy](https://github.com/SirProxy)

## Disclaimer

O uso da ferramenta ByteRecon deve ser feito com responsabilidade e sabedoria. Esta ferramenta foi desenvolvida para fins educacionais e de teste de segurança em ambientes controlados. O autor não se responsabiliza por qualquer uso indevido ou ilegal da ferramenta. 

É responsabilidade do usuário garantir que o uso do ByteRecon esteja em conformidade com todas as leis e regulamentos aplicáveis. O uso desta ferramenta contra sistemas sem permissão explícita pode ser ilegal e resultar em consequências severas. Sempre obtenha autorização antes de realizar qualquer teste de segurança.

