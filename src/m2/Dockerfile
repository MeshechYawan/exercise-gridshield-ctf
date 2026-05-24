FROM gridshield/common:public
RUN useradd -m -s /bin/bash nsadmin && echo 'nsadmin:N0v@S3c!2024' | chpasswd
COPY m2_server.py /usr/local/bin/m2_server.py
CMD ["python3", "/usr/local/bin/m2_server.py"]
