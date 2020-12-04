#include "kd_tree.h"

/*
Description: K-D Tree improved for a better data structure performance. 
Author: Yixi CAI
email: yixicai@connect.hku.hk
*/

KD_TREE::KD_TREE(float delete_param, float balance_param) {
    delete_criterion_param = delete_param;
    balance_criterion_param = balance_param;
}

KD_TREE::~KD_TREE()
{
    delete_tree_nodes(Root_Node);
}

void KD_TREE::Set_delete_criterion_param(float delete_param){
    delete_criterion_param = delete_param;
}

void KD_TREE::Set_balance_criterion_param(float balance_param){
    balance_criterion_param = balance_param;
}

void KD_TREE::Build(vector<PointType> point_cloud){
    if (Root_Node != nullptr){
        delete_tree_nodes(Root_Node);
    }
    rebuild_counter = 0;
    PCL_Storage = point_cloud;   
    BuildTree(Root_Node, 0, PCL_Storage.size()-1);
}

void KD_TREE::Nearest_Search(PointType point, int k_nearest, vector<PointType>& Nearest_Points){
    q = priority_queue<PointType_CMP> (); // Clear the priority queue;
    search_counter = 0;
    Search(Root_Node, k_nearest, point);
    // printf("Search Counter is %d ",search_counter);
    int k_found = min(k_nearest,int(q.size()));
    for (int i=0;i < k_found;i++){
        Nearest_Points.insert(Nearest_Points.begin(), q.top().point);
        q.pop();
    }
    return;
}

void KD_TREE::Add_Points(vector<PointType> PointToAdd, int PointNum){
    for (int i=0; i<PointToAdd.size();i++){
        Add(Root_Node, PointToAdd[i]);
    }
    printf("Rebuild counter is %d ", rebuild_counter);    
    return;
}

void KD_TREE::Delete_Points(vector<PointType> PointToDel, int PointNum){
    bool flag;
    for (int i=0;i!=PointToDel.size();i++){
        flag = Delete_by_point(Root_Node, PointToDel[i]);
        if (!flag) {
            printf("Failed to delete point (%0.3f,%0.3f,%0.3f)", PointToDel[i].x, PointToDel[i].y, PointToDel[i].z);
        }
    }
    return;
}

void KD_TREE::Delete_Point_Boxes(float box_x_range[][2], float box_y_range[][2], float box_z_range[][2], int Box_Number){
    for (int i=0;i<Box_Number;i++){
        Delete_by_range(Root_Node , box_x_range[i], box_y_range[i], box_z_range[i]);
    }
    return;
}

void KD_TREE::BuildTree(KD_TREE_NODE * &root, int l, int r){
    if (l>r) return;
    root = new KD_TREE_NODE;
    int mid = (l+r)>>1; 
    // Find the best division Axis
    int i;
    float average[3] = {0,0,0};
    float covariance[3] = {0,0,0};
    for (i=l;i<=r;i++){
        average[0] = average[0] + PCL_Storage[i].x;
        average[1] = average[1] + PCL_Storage[i].y;
        average[2] = average[2] + PCL_Storage[i].z;
    }
    for (i=0;i<3;i++) average[i] = average[i]/(r-l+1);
    for (i=l;i<=r;i++){
        covariance[0] = (PCL_Storage[i].x - average[0]) * (PCL_Storage[i].x - average[0]);
        covariance[1] = (PCL_Storage[i].y - average[1]) * (PCL_Storage[i].y - average[1]);  
        covariance[2] = (PCL_Storage[i].z - average[2]) * (PCL_Storage[i].z - average[2]);              
    }
    int div_axis = 0;
    for (i = 1;i<3;i++){
        if (covariance[i] > covariance[div_axis]) div_axis = i;
    }
    root->division_axis = div_axis;
    switch (div_axis)
    {
    case 0:
        nth_element(begin(PCL_Storage)+l, begin(PCL_Storage)+mid, begin(PCL_Storage)+r+1, point_cmp_x);
        break;
    case 1:
        nth_element(begin(PCL_Storage)+l, begin(PCL_Storage)+mid, begin(PCL_Storage)+r+1, point_cmp_y);
        break;
    case 2:
        nth_element(begin(PCL_Storage)+l, begin(PCL_Storage)+mid, begin(PCL_Storage)+r+1, point_cmp_z);
        break;
    default:
        nth_element(begin(PCL_Storage)+l, begin(PCL_Storage)+mid, begin(PCL_Storage)+r+1, point_cmp_x);
        break;
    }  
    root->point = PCL_Storage[mid];             
    //printf("%d %d %d\n Current Point (%0.3f, %0.3f, %0.3f) Division Axis: %d\n", l,mid,r,root->point.x, root->point.y, root->point.z, div_axis);
    BuildTree(root->left_son_ptr, l, mid-1);
    BuildTree(root->right_son_ptr, mid+1, r);  
    Update(root);
    // In the very first building tree, check is unnecessary as the balance properties is gauranteed.
    return;
}

void KD_TREE::Rebuild(KD_TREE_NODE * &root){
    // Clear the PCL_Storage vector and release memory
    // printf("Rebuilding ...\n");
    vector<PointType> ().swap(PCL_Storage);
    traverse_for_rebuild(root);
    //printf("Rebuilding %d\n",root->TreeSize);
    BuildTree(root, 0, PCL_Storage.size()-1);
    rebuild_counter += 1;
    return;
}

void KD_TREE::Delete_by_range(KD_TREE_NODE * root, float x_range[], float y_range[], float z_range[]){
    if (root == nullptr) return;
    if (x_range[0] < root->node_range_x[0] && x_range[1] > root->node_range_x[1] && y_range[0] < root->node_range_y[0] && y_range[1] > root->node_range_y[1] && z_range[0] < root->node_range_z[0] && z_range[1] > root->node_range_z[1]){
        root->tree_deleted = true;
        root->point_deleted = true;
        root->invalid_point_num = root->TreeSize;
        return;
    }
    Delete_by_range(root->left_son_ptr, x_range, y_range, z_range);
    Delete_by_range(root->right_son_ptr, x_range, y_range, z_range);
    Update(root);
    if (Criterion_Check(root)) Rebuild(root);
    return;
}

bool KD_TREE::Delete_by_point(KD_TREE_NODE * root, PointType point){
    if (root == nullptr) return false;
    bool flag = false;
    if (same_point(root->point, point) && !root->point_deleted) {
        root->point_deleted = true;
        root->invalid_point_num += 1;
        if (root->invalid_point_num == root->TreeSize) root->tree_deleted = true;
        return true;
    }
    switch (root->division_axis)
    {
    case 0:
        if (point.x < (root->point.x - EPS)) flag = Delete_by_point(root->left_son_ptr, point);
            else flag = Delete_by_point(root->right_son_ptr, point);
        break;
    case 1:
        if (point.y < (root->point.y - EPS)) flag = Delete_by_point(root->left_son_ptr, point);
        else flag = Delete_by_point(root->right_son_ptr, point);
        break;
    case 2:
        if (point.z < (root->point.z - EPS)) flag = Delete_by_point(root->left_son_ptr, point);
        else flag = Delete_by_point(root->right_son_ptr, point);
        break;
    default:
        if (point.x < (root->point.x - EPS)) flag = Delete_by_point(root->left_son_ptr, point);
            else flag = Delete_by_point(root->right_son_ptr, point);    
        break;
    }
    Update(root);
    if (Criterion_Check(root)) Rebuild(root); 
    return flag;
}

void KD_TREE::Add(KD_TREE_NODE * &root, PointType point){
    if (root == nullptr){
        root = new KD_TREE_NODE;
        root->point = point;
        Update(root);
        return;
    }
    switch (root->division_axis)
    {
    case 0:
        if (point.x < root->point.x) Add(root->left_son_ptr, point);
            else Add(root->right_son_ptr, point);
        break;
    case 1:
        if (point.y < root->point.y ) Add(root->left_son_ptr, point);
        else Add(root->right_son_ptr, point);
        break;
    case 2:
        if (point.z < root->point.z) Add(root->left_son_ptr, point);
        else Add(root->right_son_ptr, point);
        break;
    default:
        if (point.x < root->point.x) Add(root->left_son_ptr, point);
            else Add(root->right_son_ptr, point);    
        break;
    }    
    Update(root);
    root->need_rebuild = Criterion_Check(root);
    if (!root->need_rebuild){
        if (root->left_son_ptr != nullptr & root->left_son_ptr->need_rebuild) Rebuild(root->left_son_ptr);
        if (root->right_son_ptr != nullptr & root->right_son_ptr->need_rebuild) Rebuild(root->right_son_ptr);
    } else if (root == Root_Node) Rebuild(root);
    // printf("%d",root->need_rebuild);
    // if (root->left_son_ptr!=nullptr) printf(" %d",root->left_son_ptr->need_rebuild);
    // if (root->right_son_ptr!=nullptr) printf(" %d",root->right_son_ptr->need_rebuild);    
    // printf("\n");
    return;
}

void KD_TREE::Search(KD_TREE_NODE * root, int k_nearest, PointType point){
    if (root == nullptr || root->tree_deleted) return;
    search_counter += 1;
    if (!root->point_deleted){
        float dist = calc_dist(point, root->point);
        if (q.size() < k_nearest || dist < q.top().dist){
            if (q.size() >= k_nearest) q.pop();
            PointType_CMP current_point{root->point, dist};                    
            q.push(current_point);            
        }
    }
    float dist_left_node = calc_box_dist(root->left_son_ptr, point);
    float dist_right_node = calc_box_dist(root->right_son_ptr, point);
    if (q.size()< k_nearest || dist_left_node < q.top().dist && dist_right_node < q.top().dist){
        if (dist_left_node <= dist_right_node) {
            Search(root->left_son_ptr, k_nearest, point);
            if (q.size() < k_nearest || dist_right_node < q.top().dist) Search(root->right_son_ptr, k_nearest, point);
        } else {
            Search(root->right_son_ptr, k_nearest, point);
            if (q.size() < k_nearest || dist_left_node < q.top().dist) Search(root->left_son_ptr, k_nearest, point);
        }
    } else {
        if (dist_left_node < q.top().dist) Search(root->left_son_ptr, k_nearest, point);
        if (dist_right_node < q.top().dist) Search(root->right_son_ptr, k_nearest, point);
    }
    return;
}

bool KD_TREE::Criterion_Check(KD_TREE_NODE * root){
    if (root->TreeSize == 1){
        return false;
    }
    float balance_evaluation = 0.0;
    float delete_evaluation = 0.0;
    KD_TREE_NODE * son_ptr = root->left_son_ptr;
    if (son_ptr == nullptr) son_ptr = root->right_son_ptr;
    delete_evaluation = float(root->invalid_point_num)/ root->TreeSize;
    balance_evaluation = float(son_ptr->TreeSize) / root->TreeSize;
    if (delete_evaluation > delete_criterion_param){
        return true;
    }
    if (balance_evaluation > balance_criterion_param || balance_evaluation < 1-balance_criterion_param){
        return true;
    } 
    return false;
}

void KD_TREE::Update(KD_TREE_NODE* root){
    KD_TREE_NODE * left_son_ptr = root->left_son_ptr;
    KD_TREE_NODE * right_son_ptr = root->right_son_ptr;
    // Update Tree Size
    // Delete point should be considered
    if (left_son_ptr != nullptr && right_son_ptr != nullptr){
        root->TreeSize = left_son_ptr->TreeSize + right_son_ptr->TreeSize + (root->point_deleted? 0:1);
        root->invalid_point_num = left_son_ptr->invalid_point_num + right_son_ptr->invalid_point_num + (root->point_deleted? 1:0);
        root->tree_deleted = left_son_ptr->tree_deleted && right_son_ptr->tree_deleted && root->point_deleted;
        root->node_range_x[0] = min(min(left_son_ptr->node_range_x[0],right_son_ptr->node_range_x[0]),root->point.x);
        root->node_range_x[1] = max(max(left_son_ptr->node_range_x[1],right_son_ptr->node_range_x[1]),root->point.x);
        root->node_range_y[0] = min(min(left_son_ptr->node_range_y[0],right_son_ptr->node_range_y[0]),root->point.y);
        root->node_range_y[1] = max(max(left_son_ptr->node_range_y[1],right_son_ptr->node_range_y[1]),root->point.y);        
        root->node_range_z[0] = min(min(left_son_ptr->node_range_z[0],right_son_ptr->node_range_z[0]),root->point.z);
        root->node_range_z[1] = max(max(left_son_ptr->node_range_z[1],right_son_ptr->node_range_z[1]),root->point.z);         
    } else if (left_son_ptr != nullptr){
        root->TreeSize = left_son_ptr->TreeSize + (root->point_deleted? 0:1);
        root->invalid_point_num = left_son_ptr->invalid_point_num + (root->point_deleted?1:0);
        root->tree_deleted = left_son_ptr->tree_deleted && root->point_deleted;
        root->node_range_x[0] = min(left_son_ptr->node_range_x[0],root->point.x);
        root->node_range_x[1] = max(left_son_ptr->node_range_x[1],root->point.x);
        root->node_range_y[0] = min(left_son_ptr->node_range_y[0],root->point.y);
        root->node_range_y[1] = max(left_son_ptr->node_range_y[1],root->point.y); 
        root->node_range_z[0] = min(left_son_ptr->node_range_z[0],root->point.z);
        root->node_range_z[1] = max(left_son_ptr->node_range_z[1],root->point.z);               
    } else if (right_son_ptr != nullptr){
        root->TreeSize = right_son_ptr->TreeSize + (root->point_deleted? 0:1);
        root->invalid_point_num = right_son_ptr->invalid_point_num + (root->point_deleted? 1:0);
        root->tree_deleted = right_son_ptr->tree_deleted && root->point_deleted;        
        root->node_range_x[0] = min(right_son_ptr->node_range_x[0],root->point.x);
        root->node_range_x[1] = max(right_son_ptr->node_range_x[1],root->point.x);
        root->node_range_y[0] = min(right_son_ptr->node_range_y[0],root->point.y);
        root->node_range_y[1] = max(right_son_ptr->node_range_y[1],root->point.y); 
        root->node_range_z[0] = min(right_son_ptr->node_range_z[0],root->point.z);
        root->node_range_z[1] = max(right_son_ptr->node_range_z[1],root->point.z);        
    } else {
        root->TreeSize = (root->point_deleted? 0:1);
        root->invalid_point_num = (root->point_deleted? 1:0);
        root->tree_deleted = root->point_deleted;
        root->node_range_x[0] = root->point.x;
        root->node_range_x[1] = root->point.x;        
        root->node_range_y[0] = root->point.y;
        root->node_range_y[1] = root->point.y; 
        root->node_range_z[0] = root->point.z;
        root->node_range_z[1] = root->point.z;                 
    }
    return;
}

void KD_TREE::traverse_for_rebuild(KD_TREE_NODE * root){
    if (root == nullptr || root->tree_deleted) return;
    if (!root->point_deleted) PCL_Storage.push_back(root->point);
    traverse_for_rebuild(root->left_son_ptr);
    traverse_for_rebuild(root->right_son_ptr);
    return;
}

void KD_TREE::delete_tree_nodes(KD_TREE_NODE * root){
    if (root == nullptr) return;
    delete_tree_nodes(root->left_son_ptr);
    delete_tree_nodes(root->right_son_ptr);
    delete root;
    root = nullptr;
    return;
}

bool KD_TREE::same_point(PointType a, PointType b){
    return (fabs(a.x-b.x) < EPS &&fabs(a.y-b.y) < EPS && fabs(a.z-b.z) < EPS );
}

float KD_TREE::calc_dist(PointType a, PointType b){
    float dist = 0.0f;
    dist = (a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z);
    return dist;
}

float KD_TREE::calc_box_dist(KD_TREE_NODE * node, PointType point){
    if (node == nullptr) return INFINITY;
    float min_dist = 0.0;
    if (point.x < node->node_range_x[0]) min_dist += (point.x - node->node_range_x[0])*(point.x - node->node_range_x[0]);
    if (point.x > node->node_range_x[1]) min_dist += (point.x - node->node_range_x[1])*(point.x - node->node_range_x[1]);
    if (point.y < node->node_range_y[0]) min_dist += (point.y - node->node_range_y[0])*(point.y - node->node_range_y[0]);
    if (point.y > node->node_range_y[1]) min_dist += (point.y - node->node_range_y[1])*(point.y - node->node_range_y[1]);
    if (point.z < node->node_range_z[0]) min_dist += (point.z - node->node_range_z[0])*(point.z - node->node_range_z[0]);
    if (point.z > node->node_range_z[1]) min_dist += (point.z - node->node_range_z[1])*(point.z - node->node_range_z[1]);
    return min_dist;
}

bool KD_TREE::point_cmp_x(PointType a, PointType b) { return a.x < b.x;}
bool KD_TREE::point_cmp_y(PointType a, PointType b) { return a.y < b.y;}
bool KD_TREE::point_cmp_z(PointType a, PointType b) { return a.z < b.z;}
