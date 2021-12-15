/*
Non funzionano senza le variabili globali nel main.
*/

void *thread_write()
{
  int i, i_mod, size, n_pkts, totBytes = MAXLINE;

  // Calcola la dimensione del file
  fseek(fp, 0L, SEEK_END);
  size = (int) ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  // Calcola quanti blocchi serviranno
  n_pkts = size % MAXLINE ? size/MAXLINE + 1 : size/MAXLINE;

  // Popola i pacchetti
  for ( i = 0; i < n_pkts; i++)
  {     
    i_mod = (i != 0)? i%(BUFFER_SIZE) : i;
    // wait <s[i], 1>
    sem_wait(&sem_S[i_mod]);
    fseek(fp,MAXLINE*i,SEEK_SET);   
    fread(&buffer[i_mod*MAXLINE],totBytes,1,fp);
    // signal <r[i], 1>
    sem_post(&sem_R[i_mod]);
  } 
}

void *thread_send()
{ 
  int seq = 0; // primo sequence number, da mettere casuale(?).
  int j, i = 0;
  packet pack;
  
  // loop di invio. 
  while( 1)
  {
    // wait <t, 1>
    sem_wait(&sem_T);
    // wait <r[i], 1>
    sem_wait(&sem_R[i]);

    // compila il pacchetto.
    pack.seqnumb = (int32_t) seq;
    seq++;
    memcpy((void *)pack.data, (void *) &buffer[i*MAXLINE], MAXLINE*sizeof(char));
    
    // compila struttura di controllo.
    for( j = 0; j < W; j++)
    {
      if( ctrl[j].overwritable)
      {
        ctrl[j].acknumb = seq + 1;
        ctrl[j].seqnumb = seq;
        ctrl[j].position = i;
        ctrl[j].overwritable = false;
        break;
      }
    }
    i = (i + 1) % BUFFER_SIZE;
    sendto(sockfd, (const void *)&pack, sizeof(packet), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if( !timer)
    {
      // imposta un timer.
      alarm(T);
      timer = true;
    }    
  }
}


void *thread_ack()
{
  struct packet *temp = malloc(sizeof(struct packet));
  socklen_t addr_size = sizeof( struct sockaddr_in);
  int32_t ack;
  int i, aux = 0;
  while(1)
  {
    recvfrom(sockfd, temp, sizeof(packet), 0, (struct sockaddr*)&servaddr, &addr_size); //va definito bene sockfd e servaddr
    ack = temp -> acknumb;

    if (ack == ack_atteso)
    {
      for ( i = 0; i < W; i++)
      {
        if ((ctrl[i].seqnumb + 1) == ack)
        {
          ctrl[i].overwritable = true;
        }
      }
    }

    if (ack > ack_atteso)
    {
      reorder(ctrl, W);
      for ( i = 0; i < W; i++)
      {
        if (ctrl[i].seqnumb + 1 <= ack)
        {
          ctrl[i].overwritable = true;
          aux++;
        }
      }
      for ( i = 0; i < aux; i++){
        sem_post(&sem_T);
      }
      for ( i = 0; i < aux; i++){
        sem_post(&sem_S[ctrl[i].position]);
      }
      aux = 0;
    }
  } 
}