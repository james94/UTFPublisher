// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "TFPublisher.h"
#include "tf2_msgs/TFMessage.h"

// Sets default values
ATFPublisher::ATFPublisher()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// set default TF root frame
	TFRootFrameName = TEXT("map");

	// Update on tick by default
	bUseConstantPublishRate = false;

	// Use root node as blank (no relative transformations calculated with identity transform)
	bUseBlankRootNode = true;

	// Default timer delta time (s) (0 = on Tick)
	ConstantPublishRate = 0.0f;

	// ROSBridge server default values
	ServerIP = "127.0.0.1";
	ServerPORT = 9090;
}

// Called when the game starts or when spawned
void ATFPublisher::BeginPlay()
{
	Super::BeginPlay();

        // BindEventHandler();

	// Build TF tree
	BuildTFTree();

	// Create the ROSBridge handler for connecting with ROS
	ROSBridgeHandler = MakeShareable<FROSBridgeHandler>(
		new FROSBridgeHandler(ServerIP, ServerPORT));

	// Create the tf publisher
	TFPublisher = MakeShareable<FROSBridgePublisher>(
		new FROSBridgePublisher("tf", "tf2_msgs/TFMessage"));
	//TFPublisher = MakeShareable<FROSBridgePublisher>(
	//	new FROSBridgePublisher("tf_static", "tf2_msgs/TFMessage"));

	// Connect to ROS
	ROSBridgeHandler->Connect();

	// Add publisher
	ROSBridgeHandler->AddPublisher(TFPublisher);

	// Bind publish function to timer
	if (bUseConstantPublishRate)
	{
		if (ConstantPublishRate > 0.f)
		{
			// Disable tick
			SetActorTickEnabled(false);
			// Setup timer
			GetWorldTimerManager().SetTimer(TFPubTimer, this, &ATFPublisher::PublishTF, ConstantPublishRate, true);
		}
	}
	else
	{
		// Take into account the PublishRate Tag key value pair (if missing, publish on tick)
	}
}

// Called when destroyed or game stopped
void ATFPublisher::EndPlay(const EEndPlayReason::Type Reason)
{
	// Disconnect before parent ends
	ROSBridgeHandler->Disconnect();

	Super::EndPlay(Reason);
}

// Called every frame
void ATFPublisher::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Publish tf
	PublishTF();
}

// Build TF tree
void ATFPublisher::BuildTFTree()
{
	// Create root node
	TFRootNode = NewObject<UTFNode>(this);
	TFRootNode->RegisterComponent();

	if (bUseBlankRootNode)
	{
		// Init node as blank (no relative transform calculation with root children)
		TFRootNode->Init(TFRootFrameName, &TFTree);
	}
	else
	{
		// TF root node uses this actor as origin position
		// relative calculation now need to happen between root and its children
		TFRootNode->Init(TFRootFrameName, &TFTree, this);
	}

	// Initialize tree with the root node
	TFTree.Init(TFRootNode);

	// Bind root node transform function pointer (call after adding to tree)
	TFRootNode->BindTransformFunction();

	// Build tree
	TFTree.Build(GetWorld());
}

// Publish tf tree
void ATFPublisher::PublishTF()
{
	// Current time as ROS time
	FROSTime TimeNow = FROSTime::Now();

	// Create TFMessage
	TSharedPtr<tf2_msgs::TFMessage> TFMsgPtr = TFTree.GetTFMessageMsg(TimeNow, Seq);
	//UE_LOG(LogTF, Warning, TEXT(" %s \n "), *TFMsgPtr->ToString());

	// PUB
	ROSBridgeHandler->PublishMsg("/tf", TFMsgPtr);

	ROSBridgeHandler->Process();

	// Update message sequence
	Seq++;
}


// void ATFPublisher::BindEventHandler()
// {
// #if SL_WITH_SLICING
//   for (TObjectIterator<USlicingBladeComponent> Itr; Itr; ++Itr)
//     {
//       // Make sure the object is in the world
//       if (GetWorld()->ContainsActor((*Itr)->GetOwner()))
//         {
//           Blades.Add(*Itr);
//         }
//     }
//   for(auto Blade : Blades)
//     {
//       UE_LOG(LogTF, Warning, TEXT("Bind blade %s"), *Blade->GetName());
//       Blade->OnObjectCreation.AddUObject(this, &ATFPublisher::OnSLObjectCreation);
//       Blade->OnObjectDestruction.AddUObject(this, &ATFPublisher::OnSLObjectDestruction);
//     }
// #endif // SL_WITH_SLICING
// }

void ATFPublisher::AddObject(UObject* InObject)
{
  UE_LOG(LogTF, Warning, TEXT("Object created %s"), *InObject->GetName());
  TFTree.AddRootChildNode(InObject->GetName(), InObject);
}

// void ATFPublisher::OnSLObjectCreation(UObject* TransformedObject, UObject* NewSlice, float Time)
// {
//   UE_LOG(LogTF, Warning, TEXT("Object created %s"), *NewSlice->GetName());
//   UE_LOG(LogTF, Warning, TEXT("TransformedObject created %s"), *TransformedObject->GetName());
//   TFTree.AddRootChildNode(NewSlice->GetName(), NewSlice);
//   TFTree.AddRootChildNode(TransformedObject->GetName(), TransformedObject);
// }

// void ATFPublisher::OnSLObjectDestruction(UObject* ObjectActedOn, float Time)
// {
//   UE_LOG(LogTF, Warning, TEXT("BeginObject destroy %s"), *ObjectActedOn->GetName());
//   UTFNode* Node = TFTree.FindNode(ObjectActedOn->GetName());
//   if(Node)
//     {
//       UE_LOG(LogTF, Warning, TEXT("Object destroyed %s"), *ObjectActedOn->GetName());
//       TFTree.RemoveNode(Node);
//     }

// }
