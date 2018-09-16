# LTE-Sim
> TCP/UDP client-server application to simulate LTE communication

![](https://api.travis-ci.com/wychotom/pszemeg.svg?branch=master)

## Building

Client:

```sh
gcc -Wall -ggdb3 client.c ue_traffic.c messages.c setup_socket.c ue_init.c show_cell.c log.c -o client.o -DDEBUG
```

Server:
```sh
g++ *.cpp -DDEBUG
```

You can pass additional `-DLANCONN` parameter to both commands to run this app in LAN network:

```sh
gcc -Wall -ggdb3 client.c ue_traffic.c messages.c setup_socket.c ue_init.c show_cell.c log.c -o client.o -DDEBUG -DLANCONN
g++ *.cpp -DDEBUG -DLANCONN
```

## Authors

See the list of [contributors](https://github.com/wychotom/pszemeg/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details


## Contributing

1. Fork it (<https://github.com/wychotom/pszemeg/fork>)
2. Create your feature branch (`git checkout -b feature/fooBar`)
3. Commit your changes (`git commit -am 'Add some fooBar'`)
4. Push to the branch (`git push origin feature/fooBar`)
5. Create a new Pull Request
