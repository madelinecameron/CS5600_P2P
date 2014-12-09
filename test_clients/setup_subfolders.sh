#Setup
DIR="$(cd "$(dirname "$0")" && pwd)"
echo $DIR

for i in 1 2 3 4 5
do
	if [ ! -d $DIR"/client_"$i ]
	then
		echo "Creating client_"$i
		mkdir $DIR"/client_"$i
	else
		echo "Removing files from client_"$i
		rm -f $DIR"/client_"$z/*
	fi
	cd $DIR"/client_"$i
	wget http://www.photosnewhd.com/media/images/picture-wallpaper.jpg
	cd ..
done

for z in 6 7 8 9 10
do
	if [ ! -d $DIR"/client_"$z ]
	then
		echo "Creating client_"$z
		mkdir $DIR"/client_"$z
	else
		echo "Removing files from client_"$z
		rm -f $DIR"/client_"$z/*
	fi
done