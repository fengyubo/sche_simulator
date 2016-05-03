# sche_simulator
By Yubo Feng (yuf24@pitt.edu)
open source real time simulator, welcome to use!

# what is this project about?
This project is a real time scheduler simulator framework, you can implement all kind of scheduling algorithm, and plugin into this framework then test it or run experiments. This project will make researcher's work more easier.

# language of this project
C++11 standard. For some features, I use C++ STL to implement, so if there is no g++ on your computer, then you cannot even compile it.

# outer libary required
This project will use some features from libconfig (http://www.hyperrealm.com/libconfig/). Please install and use this lib before your compile it. Version of libconfig that I use is v1.5. You can find file libconfig-1.5.tar.gz in this website. Install and use it will be sufficient.

# how to compile the project?
After clone this project, then use terminal and cd to project folder, Use ./Makefile to compile will be sufficient. I develop this project in my mac, so it should work just fine in Mac OS; for general Linux should also be OK, but for Window, I haven't test it. (Basic configure for my MacOS: g++ compiler version 4.2.1, LLVM version 7.3.0 , x86_64-apple-darwin15.4.0)

# Where to start? 
Please read file sche_simulator_Guidance, it describes all things about this project. Design concern, examples, structure and so on. Please carefully read it. If there is anything that is not clear, please email me. Start reading from sched.h, sched_bfair.h and sched_bfair.cpp is a good idea, since if you uncomment codes in the bottom of sched_bfair.cpp then compile these three file, you will find that these three files could consist a simple scheduler by themselves! So, it is simple and easy to understand, from there should be a good start.

# what is next?
I will keep update for this project as long as I can, basically what I will do is: 1) update document sche_simulator_Guidance for more detail and explaination, help more developer to use. 2) bug fix, if there is anything wrong, I will fix it and update. 3) different types of scheduling algorithms, energy model and task set generator, I hope I could collect different kind of algorithms, models, generator, s.t I could put them all, so for some researchers in the future, he will no longer need to implement the algorithms that supported in this project, but more focus on his work.

